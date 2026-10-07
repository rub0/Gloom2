#pragma once

#include <gloom/render/material_surface.hpp>
#include <gloom/core/matrix.hpp>
#include <gloom/core/span.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <limits>

#include <string>
#include <vector>

namespace gloom::assets {

inline constexpr uint32 no_asset_index = std::numeric_limits<uint32>::max();

struct ImportedVertex {
    std::array<float, 3> position{};
    std::array<float, 3> normal{0.0F, 1.0F, 0.0F};
    std::array<float, 2> texture_coordinate{};
    std::array<float, 4> tangent{1.0F, 0.0F, 0.0F, 1.0F};
    std::array<float, 2> texture_coordinate_1{};
    std::array<uint16, 8> joints{};
    std::array<float, 8> weights{};
};

struct ImportedPrimitive {
    std::vector<ImportedVertex> vertices;
    std::vector<uint32> indices;
    std::vector<std::vector<uint32>> lod_indices;
    std::array<float, 3> bounds_center{};
    float bounds_radius{0.0F};
    uint32 material{no_asset_index};
};

struct ImportedMesh {
    std::string name;
    uint32 first_primitive{0};
    uint32 primitive_count{0};
};

struct ImportedMaterial {
    std::string name;
    std::array<float, 4> base_color{1.0F, 1.0F, 1.0F, 1.0F};
    std::array<float, 3> emissive{};
    float metallic{1.0F};
    float roughness{1.0F};
    float alpha_cutoff{0.5F};
    uint8 alpha_mode{0};
    bool double_sided{false};
    uint32 base_color_texture{no_asset_index};
    uint32 metallic_roughness_texture{no_asset_index};
    uint32 normal_texture{no_asset_index};
    render::MaterialSurface surface;
    std::array<uint32, 7> extra_textures{no_asset_index, no_asset_index, no_asset_index, no_asset_index, no_asset_index, no_asset_index, no_asset_index};
};

[[nodiscard]] inline std::array<uint32, render::material_texture_count> material_textures(const ImportedMaterial& material) {
    return {material.base_color_texture, material.metallic_roughness_texture, material.normal_texture, material.extra_textures[0], material.extra_textures[1],
        material.extra_textures[2], material.extra_textures[3], material.extra_textures[4], material.extra_textures[5], material.extra_textures[6]};
}

struct ImportedTexture {
    uint32 image{no_asset_index};
};

struct ImportedImage {
    std::string name;
    std::string external_uri;
    bool embedded{false};
};

struct ImportedNode {
    std::string name;
    Matrix4 local_transform{.values = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
    uint32 mesh{no_asset_index};
    std::vector<uint32> children;
    uint32 skin{no_asset_index};
};

struct ImportedSkin {
    std::string name;
    std::vector<uint32> joints;
    std::vector<Matrix4> inverse_bind_matrices;
};

enum class AnimationPath : uint8 { translation, rotation, scale };
enum class AnimationInterpolation : uint8 { linear, step };

struct AnimationChannel {
    uint32 node{no_asset_index};
    AnimationPath path{AnimationPath::translation};
    AnimationInterpolation interpolation{AnimationInterpolation::linear};
    std::vector<float> times;
    std::vector<std::array<float, 4>> values;
};

struct AnimationClip {
    std::string name;
    float duration{0};
    std::vector<AnimationChannel> channels;
};

struct ImportedSceneDefinition {
    std::string name;
    std::vector<uint32> roots;
};

struct ImportedScene {
    uint32 default_scene{no_asset_index};
    std::vector<ImportedPrimitive> primitives;
    std::vector<ImportedMesh> meshes;
    std::vector<ImportedMaterial> materials;
    std::vector<ImportedTexture> textures;
    std::vector<ImportedImage> images;
    std::vector<ImportedNode> nodes;
    std::vector<ImportedSceneDefinition> scenes;
    std::vector<ImportedSkin> skins;
    std::vector<AnimationClip> animations;
};

[[nodiscard]] std::expected<ImportedScene, std::string> import_gltf(const std::filesystem::path& source);
// Strings and containers own their data; parser views never escape import_gltf.
// validate_imported_scene checks external/edited scenes before encoding. Encoder requires a valid scene.
[[nodiscard]] bool validate_imported_scene(const ImportedScene& scene);
[[nodiscard]] std::vector<std::byte> encode_imported_scene(const ImportedScene& scene);
[[nodiscard]] std::expected<ImportedScene, std::string> decode_imported_scene(Span<const std::byte> encoded);

} // namespace gloom::assets
