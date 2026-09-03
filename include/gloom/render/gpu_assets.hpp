#pragma once

#include <gloom/render/material_surface.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gloom::render {

struct RenderAssetId {
    std::uint64_t value{0};
    [[nodiscard]] bool operator==(const RenderAssetId&) const noexcept = default;
};

struct RenderAssetIdHash {
    [[nodiscard]] std::size_t operator()(const RenderAssetId id) const noexcept {
        return static_cast<std::size_t>(id.value ^ (id.value >> 32U));
    }
};

inline constexpr RenderAssetId builtin_cube_mesh{1};
inline constexpr RenderAssetId builtin_default_material{2};
inline constexpr RenderAssetId builtin_white_texture{3};
inline constexpr RenderAssetId builtin_flat_normal_texture{4};
inline constexpr RenderAssetId builtin_horizontal_quad_mesh{5};

struct GpuVertex {
    std::array<float, 3> position{};
    std::array<float, 3> normal{0.0F, 1.0F, 0.0F};
    std::array<float, 2> texture_coordinate{};
    std::array<float, 4> tangent{1.0F, 0.0F, 0.0F, 1.0F};
    std::array<float, 2> texture_coordinate_1{};
    std::array<std::uint16_t, 8> joints{};
    std::array<float, 8> weights{};
};

struct MeshUpload {
    RenderAssetId id;
    std::vector<GpuVertex> vertices;
    std::vector<std::uint32_t> indices;
};

enum class TextureFormat : std::uint8_t { rgba8, bc5, bc7 };

struct TextureUpload {
    struct MipLevel {
        std::uint32_t width{1};
        std::uint32_t height{1};
        std::vector<std::byte> data;
    };

    RenderAssetId id;
    TextureFormat format{TextureFormat::rgba8};
    bool srgb{true};
    std::vector<MipLevel> mip_levels;
};

struct MaterialUpload {
    RenderAssetId id;
    std::array<float, 4> base_color{1.0F, 1.0F, 1.0F, 1.0F};
    std::array<float, 3> emissive{};
    float metallic{0.0F};
    float roughness{0.8F};
    RenderAssetId base_color_texture{builtin_white_texture};
    RenderAssetId metallic_roughness_texture{builtin_white_texture};
    RenderAssetId normal_texture{builtin_flat_normal_texture};
    MaterialSurface surface;
    std::array<RenderAssetId, 7> extra_textures{
        builtin_white_texture, builtin_white_texture, builtin_white_texture,
        builtin_white_texture, builtin_white_texture, builtin_white_texture,
        builtin_white_texture};
};

enum class GpuAssetState : std::uint8_t { missing, queued, resident, failed };

struct GpuResidencyMetrics {
    std::uint64_t queued_bytes{0};
    std::uint64_t uploaded_bytes{0};
    std::uint64_t resident_bytes{0};
    std::uint64_t deferred_releases{0};
    std::uint64_t completed_releases{0};
    std::uint64_t evictions{0};
    std::uint64_t budget_limited_frames{0};
};

} // namespace gloom::render
