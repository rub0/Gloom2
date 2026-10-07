#include <gloom/assets/texture_asset.hpp>
#include <gloom/core/types.hpp>

#include <ktx.h>
#if defined(GLOOM_TEXTURE_COOKER)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#include <math.h>
#include <cstdlib>
#include <limits>
#include <memory>
#include <thread>

namespace gloom::assets {
namespace {

// Stable VkFormat numeric values from the Vulkan registry. Keeping these local
// avoids making the offline texture cooker depend on Vulkan SDK headers.
constexpr uint32 vk_format_rgba8_unorm = 37;
constexpr uint32 vk_format_rgba8_srgb = 43;

#if defined(GLOOM_TEXTURE_COOKER)
struct StbiDeleter {
    void operator()(stbi_uc* pixels) const noexcept {
        stbi_image_free(pixels);
    }
};
#endif

struct KtxDeleter {
    void operator()(ktxTexture2* texture) const noexcept {
        if (texture != nullptr) {
            ktxTexture_Destroy(ktxTexture(texture));
        }
    }
};

[[nodiscard]] std::string ktx_error(const std::string_view operation, const KTX_error_code code) {
    return std::string{operation} + ": " + ktxErrorString(code);
}

#if defined(GLOOM_TEXTURE_COOKER)
[[nodiscard]] uint8 filtered_channel(const uint32 sum, const uint32 samples) noexcept {
    return static_cast<uint8>((sum + samples / 2U) / samples);
}

[[nodiscard]] std::vector<uint8> downsample(const Span<const uint8> source, const uint32 width, const uint32 height, const TextureSemantic semantic) {
    const uint32 next_width = (width > 1 ? width / 2U : 1U);
    const uint32 next_height = (height > 1 ? height / 2U : 1U);
    std::vector<uint8> result(static_cast<size_t>(next_width) * next_height * 4U);
    for (uint32 y = 0; y < next_height; ++y) {
        for (uint32 x = 0; x < next_width; ++x) {
            uint32 sums[4]{};
            float linear_rgb[3]{};
            uint32 samples = 0;
            for (uint32 offset_y = 0; offset_y < 2; ++offset_y) {
                const uint32 source_y = (y * 2U + offset_y < height ? y * 2U + offset_y : height - 1U);
                for (uint32 offset_x = 0; offset_x < 2; ++offset_x) {
                    const uint32 source_x = (x * 2U + offset_x < width ? x * 2U + offset_x : width - 1U);
                    const size_t source_offset = (static_cast<size_t>(source_y) * width + source_x) * 4U;
                    for (uint32 channel = 0; channel < 4; ++channel) {
                        sums[channel] += source[source_offset + channel];
                    }
                    if (semantic == TextureSemantic::color) {
                        for (uint32 channel = 0; channel < 3; ++channel) {
                            const float encoded = source[source_offset + channel] / 255.0F;
                            linear_rgb[channel] += encoded <= 0.04045F ? encoded / 12.92F : powf((encoded + 0.055F) / 1.055F, 2.4F);
                        }
                    }
                    ++samples;
                }
            }
            const size_t destination = (static_cast<size_t>(y) * next_width + x) * 4U;
            for (uint32 channel = 0; channel < 3; ++channel) {
                if (semantic == TextureSemantic::color) {
                    const float linear = linear_rgb[channel] / static_cast<float>(samples);
                    const float encoded = linear <= 0.0031308F ? linear * 12.92F : 1.055F * powf(linear, 1.0F / 2.4F) - 0.055F;
                    result[destination + channel] = static_cast<uint8>(fminf(fmaxf(encoded * 255.0F + 0.5F, 0.0F), 255.0F));
                } else {
                    result[destination + channel] = filtered_channel(sums[channel], samples);
                }
            }
            result[destination + 3] = filtered_channel(sums[3], samples);
        }
    }
    return result;
}
#endif

} // namespace

#if defined(GLOOM_TEXTURE_COOKER)
std::expected<std::vector<std::byte>, std::string> cook_texture_ktx2(const Span<const std::byte> encoded_image, const TextureSemantic semantic) {
    if (encoded_image.empty() || encoded_image.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return std::unexpected{"Encoded source image is empty or too large"};
    }
    int width = 0;
    int height = 0;
    int source_channels = 0;
    std::unique_ptr<stbi_uc, StbiDeleter> pixels{stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(encoded_image.data()), static_cast<int>(encoded_image.size()), &width, &height, &source_channels, 4)};
    if (!pixels || width <= 0 || height <= 0) {
        return std::unexpected{"Could not decode source texture: " + std::string{stbi_failure_reason()}};
    }

    uint32 level_count = 1;
    uint32 mip_width = static_cast<uint32>(width);
    uint32 mip_height = static_cast<uint32>(height);
    while (mip_width > 1 || mip_height > 1) {
        ++level_count;
        mip_width = (mip_width > 1 ? mip_width / 2U : 1U);
        mip_height = (mip_height > 1 ? mip_height / 2U : 1U);
    }

    ktxTextureCreateInfo info{};
    info.vkFormat = semantic == TextureSemantic::color ? vk_format_rgba8_srgb : vk_format_rgba8_unorm;
    info.baseWidth = static_cast<ktx_uint32_t>(width);
    info.baseHeight = static_cast<ktx_uint32_t>(height);
    info.baseDepth = 1;
    info.numDimensions = 2;
    info.numLevels = level_count;
    info.numLayers = 1;
    info.numFaces = 1;
    ktxTexture2* raw_texture = nullptr;
    if (const KTX_error_code code = ktxTexture2_Create(&info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &raw_texture); code != KTX_SUCCESS) {
        return std::unexpected{ktx_error("Could not create KTX2 texture", code)};
    }
    std::unique_ptr<ktxTexture2, KtxDeleter> texture{raw_texture};
    {
        // KTX owns each populated level. Keep only the previous mip while filtering, then free staging before compression.
        Span<const uint8> previous{pixels.get(), static_cast<size_t>(width) * height * 4U};
        std::vector<uint8> mip;
        mip_width = static_cast<uint32>(width);
        mip_height = static_cast<uint32>(height);
        for (uint32 level = 0; level < level_count; ++level) {
            if (const KTX_error_code code = ktxTexture_SetImageFromMemory(ktxTexture(texture.get()), level, 0, 0, previous.data(), previous.size());
                code != KTX_SUCCESS) {
                return std::unexpected{ktx_error("Could not populate KTX2 mip level", code)};
            }
            if (level + 1 < level_count) {
                mip = downsample(previous, mip_width, mip_height, semantic);
                previous = mip;
                mip_width = mip_width > 1 ? mip_width / 2U : 1U;
                mip_height = mip_height > 1 ? mip_height / 2U : 1U;
                pixels.reset();
            }
        }
    }
    pixels.reset();

    ktxBasisParams parameters{};
    parameters.structSize = sizeof(parameters);
    // UASTC retains authored specular/detail maps and avoids ETC1S codebook
    // optimization dominating full-resolution legacy scene builds.
    parameters.uastc = KTX_TRUE;
    parameters.normalMap = semantic == TextureSemantic::normal ? KTX_TRUE : KTX_FALSE;
    parameters.qualityLevel = 128;
    parameters.compressionLevel = 2;
    parameters.threadCount = std::thread::hardware_concurrency();
    if (!parameters.threadCount)
        parameters.threadCount = 1;
    if (const KTX_error_code code = ktxTexture2_CompressBasisEx(texture.get(), &parameters); code != KTX_SUCCESS) {
        return std::unexpected{ktx_error("Could not Basis-compress KTX2 texture", code)};
    }

    ktx_uint8_t* encoded = nullptr;
    ktx_size_t encoded_size = 0;
    if (const KTX_error_code code = ktxTexture_WriteToMemory(ktxTexture(texture.get()), &encoded, &encoded_size); code != KTX_SUCCESS) {
        return std::unexpected{ktx_error("Could not serialize KTX2 texture", code)};
    }
    std::unique_ptr<ktx_uint8_t, decltype(&std::free)> encoded_owner{encoded, &std::free};
    const std::byte* begin = reinterpret_cast<const std::byte*>(encoded);
    return std::vector<std::byte>{begin, begin + encoded_size};
}
#else
std::expected<render::TextureUpload, std::string> decode_texture_ktx2(
    const render::RenderAssetId id, const Span<const std::byte> ktx2_payload, const TextureTranscodeTarget target) {
    if (id.value == 0 || ktx2_payload.empty()) {
        return std::unexpected{"KTX2 upload has an invalid ID or empty payload"};
    }
    ktxTexture2* raw_texture = nullptr;
    const KTX_error_code create_code = ktxTexture2_CreateFromMemory(
        reinterpret_cast<const ktx_uint8_t*>(ktx2_payload.data()), ktx2_payload.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &raw_texture);
    if (create_code != KTX_SUCCESS) {
        return std::unexpected{ktx_error("Could not decode KTX2 texture", create_code)};
    }
    std::unique_ptr<ktxTexture2, KtxDeleter> texture{raw_texture};
    if (texture->numDimensions != 2 || texture->isArray || texture->isCubemap || texture->baseDepth != 1 || texture->numLevels == 0) {
        return std::unexpected{"Only non-array 2D KTX2 textures are supported"};
    }
    const bool srgb = ktxTexture2_GetOETF_e(texture.get()) == KHR_DF_TRANSFER_SRGB;
    // UASTC normal maps store XY as RRRG. BC5 consumes RA, but RGBA8 expands
    // those channels literally; restore G for the material shader's XY sample.
    const bool unpack_normal_rg = target == TextureTranscodeTarget::rgba8 && KHR_DFDVAL(texture->pDfd + 1, MODEL) == KHR_DF_MODEL_UASTC &&
                                  KHR_DFDSVAL(texture->pDfd + 1, 0, CHANNELID) == KHR_DF_CHANNEL_UASTC_RRRG;
    const ktx_transcode_fmt_e transcode_format = target == TextureTranscodeTarget::bc5   ? KTX_TTF_BC5_RG
                                                 : target == TextureTranscodeTarget::bc7 ? KTX_TTF_BC7_RGBA
                                                                                         : KTX_TTF_RGBA32;
    if (ktxTexture2_NeedsTranscoding(texture.get())) {
        if (const KTX_error_code code = ktxTexture2_TranscodeBasis(texture.get(), transcode_format, 0); code != KTX_SUCCESS) {
            return std::unexpected{ktx_error("Could not transcode Basis texture", code)};
        }
    }

    const render::TextureFormat output_format = target == TextureTranscodeTarget::bc5   ? render::TextureFormat::bc5
                                                : target == TextureTranscodeTarget::bc7 ? render::TextureFormat::bc7
                                                                                        : render::TextureFormat::rgba8;
    render::TextureUpload upload{.id = id, .format = output_format, .srgb = output_format == render::TextureFormat::bc5 ? false : srgb};
    upload.mip_levels.reserve(texture->numLevels);
    uint32 width = texture->baseWidth;
    uint32 height = texture->baseHeight;
    for (uint32 level = 0; level < texture->numLevels; ++level) {
        ktx_size_t offset = 0;
        if (const KTX_error_code code = ktxTexture_GetImageOffset(ktxTexture(texture.get()), level, 0, 0, &offset); code != KTX_SUCCESS) {
            return std::unexpected{ktx_error("Could not locate KTX2 mip level", code)};
        }
        const size_t size = static_cast<size_t>(ktxTexture_GetImageSize(ktxTexture(texture.get()), level));
        if (offset > texture->dataSize || size > texture->dataSize - offset) {
            return std::unexpected{"KTX2 mip data exceeds its decoded storage"};
        }
        if (unpack_normal_rg) {
            for (uint64 pixel = 0; pixel < size; pixel += 4)
                texture->pData[offset + pixel + 1] = texture->pData[offset + pixel + 3];
        }
        const std::byte* begin = reinterpret_cast<const std::byte*>(texture->pData + offset);
        upload.mip_levels.push_back({.width = width, .height = height, .data = {begin, begin + size}});
        width = (width > 1 ? width / 2U : 1U);
        height = (height > 1 ? height / 2U : 1U);
    }
    return upload;
}
#endif

} // namespace gloom::assets
