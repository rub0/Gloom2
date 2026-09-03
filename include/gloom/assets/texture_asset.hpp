#pragma once

#include <gloom/render/gpu_assets.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace gloom::assets {

enum class TextureSemantic : std::uint8_t { color, normal, data };
enum class TextureTranscodeTarget : std::uint8_t { rgba8, bc5, bc7 };

// Offline-only authoring conversion. The returned bytes are a complete KTX2
// Basis Universal texture including all mip levels.
[[nodiscard]] std::expected<std::vector<std::byte>, std::string>
cook_texture_ktx2(std::span<const std::byte> encoded_image,
                  TextureSemantic semantic);

// Runtime conversion selects a native GPU payload supported by the active
// device. RGBA8 remains the universal fallback.
[[nodiscard]] std::expected<render::TextureUpload, std::string>
decode_texture_ktx2(render::RenderAssetId id,
                    std::span<const std::byte> ktx2_payload,
                    TextureTranscodeTarget target = TextureTranscodeTarget::rgba8);

} // namespace gloom::assets
