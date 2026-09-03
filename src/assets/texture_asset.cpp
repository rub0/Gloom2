#include <gloom/assets/texture_asset.hpp>

#include <ktx.h>
#if defined(GLOOM_TEXTURE_COOKER)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <thread>

namespace gloom::assets {
namespace {

// Stable VkFormat numeric values from the Vulkan registry. Keeping these local
// avoids making the offline texture cooker depend on Vulkan SDK headers.
constexpr std::uint32_t vk_format_rgba8_unorm = 37;
constexpr std::uint32_t vk_format_rgba8_srgb = 43;

#if defined(GLOOM_TEXTURE_COOKER)
struct StbiDeleter {
    void operator()(stbi_uc* pixels) const noexcept { stbi_image_free(pixels); }
};
#endif

struct KtxDeleter {
    void operator()(ktxTexture2* texture) const noexcept {
        if (texture != nullptr) {
            ktxTexture_Destroy(ktxTexture(texture));
        }
    }
};

[[nodiscard]] std::string ktx_error(const std::string_view operation,
                                    const KTX_error_code code) {
    return std::string{operation} + ": " + ktxErrorString(code);
}

#if defined(GLOOM_TEXTURE_COOKER)
[[nodiscard]] std::uint8_t filtered_channel(const std::uint32_t sum,
                                            const std::uint32_t samples) noexcept {
    return static_cast<std::uint8_t>((sum + samples / 2U) / samples);
}

[[nodiscard]] std::vector<std::uint8_t>
downsample(const std::vector<std::uint8_t>& source,
           const std::uint32_t width,
           const std::uint32_t height,
           const TextureSemantic semantic) {
    const std::uint32_t next_width = std::max(width / 2U, 1U);
    const std::uint32_t next_height = std::max(height / 2U, 1U);
    std::vector<std::uint8_t> result(static_cast<std::size_t>(next_width) * next_height * 4U);
    for (std::uint32_t y = 0; y < next_height; ++y) {
        for (std::uint32_t x = 0; x < next_width; ++x) {
            std::uint32_t sums[4]{};
            float linear_rgb[3]{};
            std::uint32_t samples = 0;
            for (std::uint32_t offset_y = 0; offset_y < 2; ++offset_y) {
                const auto source_y = std::min(y * 2U + offset_y, height - 1U);
                for (std::uint32_t offset_x = 0; offset_x < 2; ++offset_x) {
                    const auto source_x = std::min(x * 2U + offset_x, width - 1U);
                    const auto source_offset =
                        (static_cast<std::size_t>(source_y) * width + source_x) * 4U;
                    for (std::uint32_t channel = 0; channel < 4; ++channel) {
                        sums[channel] += source[source_offset + channel];
                    }
                    if (semantic == TextureSemantic::color) {
                        for (std::uint32_t channel = 0; channel < 3; ++channel) {
                            const float encoded = source[source_offset + channel] / 255.0F;
                            linear_rgb[channel] += encoded <= 0.04045F
                                                       ? encoded / 12.92F
                                                       : std::pow((encoded + 0.055F) / 1.055F, 2.4F);
                        }
                    }
                    ++samples;
                }
            }
            const auto destination =
                (static_cast<std::size_t>(y) * next_width + x) * 4U;
            for (std::uint32_t channel = 0; channel < 3; ++channel) {
                if (semantic == TextureSemantic::color) {
                    const float linear = linear_rgb[channel] / static_cast<float>(samples);
                    const float encoded = linear <= 0.0031308F
                                              ? linear * 12.92F
                                              : 1.055F * std::pow(linear, 1.0F / 2.4F) - 0.055F;
                    result[destination + channel] = static_cast<std::uint8_t>(
                        std::clamp(encoded * 255.0F + 0.5F, 0.0F, 255.0F));
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
std::expected<std::vector<std::byte>, std::string>
cook_texture_ktx2(const std::span<const std::byte> encoded_image,
                  const TextureSemantic semantic) {
    if (encoded_image.empty() ||
        encoded_image.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return std::unexpected{"Encoded source image is empty or too large"};
    }
    int width = 0;
    int height = 0;
    int source_channels = 0;
    std::unique_ptr<stbi_uc, StbiDeleter> pixels{stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(encoded_image.data()),
        static_cast<int>(encoded_image.size()),
        &width,
        &height,
        &source_channels,
        4)};
    if (!pixels || width <= 0 || height <= 0) {
        return std::unexpected{"Could not decode source texture: " +
                               std::string{stbi_failure_reason()}};
    }

    std::vector<std::vector<std::uint8_t>> levels;
    levels.emplace_back(pixels.get(), pixels.get() + static_cast<std::size_t>(width) * height * 4U);
    std::uint32_t mip_width = static_cast<std::uint32_t>(width);
    std::uint32_t mip_height = static_cast<std::uint32_t>(height);
    while (mip_width > 1 || mip_height > 1) {
        levels.push_back(downsample(levels.back(), mip_width, mip_height, semantic));
        mip_width = std::max(mip_width / 2U, 1U);
        mip_height = std::max(mip_height / 2U, 1U);
    }

    ktxTextureCreateInfo info{};
    info.vkFormat = semantic == TextureSemantic::color ? vk_format_rgba8_srgb
                                                        : vk_format_rgba8_unorm;
    info.baseWidth = static_cast<ktx_uint32_t>(width);
    info.baseHeight = static_cast<ktx_uint32_t>(height);
    info.baseDepth = 1;
    info.numDimensions = 2;
    info.numLevels = static_cast<ktx_uint32_t>(levels.size());
    info.numLayers = 1;
    info.numFaces = 1;
    ktxTexture2* raw_texture = nullptr;
    if (const auto code =
            ktxTexture2_Create(&info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &raw_texture);
        code != KTX_SUCCESS) {
        return std::unexpected{ktx_error("Could not create KTX2 texture", code)};
    }
    std::unique_ptr<ktxTexture2, KtxDeleter> texture{raw_texture};
    for (std::uint32_t level = 0; level < levels.size(); ++level) {
        if (const auto code = ktxTexture_SetImageFromMemory(ktxTexture(texture.get()),
                                                            level,
                                                            0,
                                                            0,
                                                            levels[level].data(),
                                                            levels[level].size());
            code != KTX_SUCCESS) {
            return std::unexpected{ktx_error("Could not populate KTX2 mip level", code)};
        }
    }

    ktxBasisParams parameters{};
    parameters.structSize = sizeof(parameters);
    // UASTC retains authored specular/detail maps and avoids ETC1S codebook
    // optimization dominating full-resolution legacy scene builds.
    parameters.uastc = KTX_TRUE;
    parameters.normalMap = semantic == TextureSemantic::normal ? KTX_TRUE : KTX_FALSE;
    parameters.qualityLevel = 128;
    parameters.compressionLevel = 2;
    parameters.threadCount = std::max(std::thread::hardware_concurrency(), 1U);
    if (const auto code = ktxTexture2_CompressBasisEx(texture.get(), &parameters);
        code != KTX_SUCCESS) {
        return std::unexpected{ktx_error("Could not Basis-compress KTX2 texture", code)};
    }

    ktx_uint8_t* encoded = nullptr;
    ktx_size_t encoded_size = 0;
    if (const auto code =
            ktxTexture_WriteToMemory(ktxTexture(texture.get()), &encoded, &encoded_size);
        code != KTX_SUCCESS) {
        return std::unexpected{ktx_error("Could not serialize KTX2 texture", code)};
    }
    std::unique_ptr<ktx_uint8_t, decltype(&std::free)> encoded_owner{encoded, &std::free};
    const auto* begin = reinterpret_cast<const std::byte*>(encoded);
    return std::vector<std::byte>{begin, begin + encoded_size};
}
#else
std::expected<render::TextureUpload, std::string>
decode_texture_ktx2(const render::RenderAssetId id,
                    const std::span<const std::byte> ktx2_payload,
                    const TextureTranscodeTarget target) {
    if (id.value == 0 || ktx2_payload.empty()) {
        return std::unexpected{"KTX2 upload has an invalid ID or empty payload"};
    }
    ktxTexture2* raw_texture = nullptr;
    const auto create_code = ktxTexture2_CreateFromMemory(
        reinterpret_cast<const ktx_uint8_t*>(ktx2_payload.data()),
        ktx2_payload.size(),
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &raw_texture);
    if (create_code != KTX_SUCCESS) {
        return std::unexpected{ktx_error("Could not decode KTX2 texture", create_code)};
    }
    std::unique_ptr<ktxTexture2, KtxDeleter> texture{raw_texture};
    if (texture->numDimensions != 2 || texture->isArray || texture->isCubemap ||
        texture->baseDepth != 1 || texture->numLevels == 0) {
        return std::unexpected{"Only non-array 2D KTX2 textures are supported"};
    }
    const bool srgb = ktxTexture2_GetOETF_e(texture.get()) == KHR_DF_TRANSFER_SRGB;
    const auto transcode_format = [&] {
        switch (target) {
        case TextureTranscodeTarget::bc5:
            return KTX_TTF_BC5_RG;
        case TextureTranscodeTarget::bc7:
            return KTX_TTF_BC7_RGBA;
        case TextureTranscodeTarget::rgba8:
            return KTX_TTF_RGBA32;
        }
        return KTX_TTF_RGBA32;
    }();
    if (ktxTexture2_NeedsTranscoding(texture.get())) {
        if (const auto code = ktxTexture2_TranscodeBasis(texture.get(), transcode_format, 0);
            code != KTX_SUCCESS) {
            return std::unexpected{ktx_error("Could not transcode Basis texture", code)};
        }
    }

    const auto output_format = target == TextureTranscodeTarget::bc5
                                   ? render::TextureFormat::bc5
                               : target == TextureTranscodeTarget::bc7
                                   ? render::TextureFormat::bc7
                                   : render::TextureFormat::rgba8;
    render::TextureUpload upload{.id = id,
                                 .format = output_format,
                                 .srgb = output_format == render::TextureFormat::bc5 ? false
                                                                                     : srgb};
    upload.mip_levels.reserve(texture->numLevels);
    std::uint32_t width = texture->baseWidth;
    std::uint32_t height = texture->baseHeight;
    for (std::uint32_t level = 0; level < texture->numLevels; ++level) {
        ktx_size_t offset = 0;
        if (const auto code =
                ktxTexture_GetImageOffset(ktxTexture(texture.get()), level, 0, 0, &offset);
            code != KTX_SUCCESS) {
            return std::unexpected{ktx_error("Could not locate KTX2 mip level", code)};
        }
        const std::size_t size = static_cast<std::size_t>(
            ktxTexture_GetImageSize(ktxTexture(texture.get()), level));
        if (offset > texture->dataSize || size > texture->dataSize - offset) {
            return std::unexpected{"KTX2 mip data exceeds its decoded storage"};
        }
        const auto* begin = reinterpret_cast<const std::byte*>(texture->pData + offset);
        upload.mip_levels.push_back(
            {.width = width, .height = height, .data = {begin, begin + size}});
        width = std::max(width / 2U, 1U);
        height = std::max(height / 2U, 1U);
    }
    return upload;
}
#endif

} // namespace gloom::assets
