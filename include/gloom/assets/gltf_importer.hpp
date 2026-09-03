#pragma once

#include <gloom/render/material_surface.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace gloom::assets {

inline constexpr std::uint32_t no_asset_index =
    std::numeric_limits<std::uint32_t>::max();

struct ImportedVertex {
  std::array<float, 3> position{};
  std::array<float, 3> normal{0.0F, 1.0F, 0.0F};
  std::array<float, 2> texture_coordinate{};
  std::array<float, 4> tangent{1.0F, 0.0F, 0.0F, 1.0F};
  std::array<float, 2> texture_coordinate_1{};
  std::array<std::uint16_t, 8> joints{};
  std::array<float, 8> weights{};
};

struct ImportedPrimitive {
  std::vector<ImportedVertex> vertices;
  std::vector<std::uint32_t> indices;
  std::vector<std::vector<std::uint32_t>> lod_indices;
  std::array<float, 3> bounds_center{};
  float bounds_radius{0.0F};
  std::uint32_t material{no_asset_index};
};

struct ImportedMesh {
  std::string name;
  std::uint32_t first_primitive{0};
  std::uint32_t primitive_count{0};
};

struct ImportedMaterial {
  std::string name;
  std::array<float, 4> base_color{1.0F, 1.0F, 1.0F, 1.0F};
  std::array<float, 3> emissive{};
  float metallic{1.0F};
  float roughness{1.0F};
  float alpha_cutoff{0.5F};
  std::uint8_t alpha_mode{0};
  bool double_sided{false};
  std::uint32_t base_color_texture{no_asset_index};
  std::uint32_t metallic_roughness_texture{no_asset_index};
  std::uint32_t normal_texture{no_asset_index};
  render::MaterialSurface surface;
  std::array<std::uint32_t, 7> extra_textures{
      no_asset_index, no_asset_index, no_asset_index, no_asset_index,
      no_asset_index, no_asset_index, no_asset_index};
};

[[nodiscard]] inline std::array<std::uint32_t, render::material_texture_count>
material_textures(const ImportedMaterial& material) {
    return {material.base_color_texture, material.metallic_roughness_texture,
            material.normal_texture, material.extra_textures[0], material.extra_textures[1],
            material.extra_textures[2], material.extra_textures[3], material.extra_textures[4],
            material.extra_textures[5], material.extra_textures[6]};
}

struct ImportedTexture {
  std::uint32_t image{no_asset_index};
};

struct ImportedImage {
  std::string name;
  std::string external_uri;
  bool embedded{false};
};

struct ImportedNode {
  std::string name;
  std::array<float, 16> local_transform{};
  std::uint32_t mesh{no_asset_index};
  std::vector<std::uint32_t> children;
  std::uint32_t skin{no_asset_index};
};

struct ImportedSkin {
  std::string name;
  std::vector<std::uint32_t> joints;
  std::vector<std::array<float, 16>> inverse_bind_matrices;
};

enum class AnimationPath : std::uint8_t { translation, rotation, scale };
enum class AnimationInterpolation : std::uint8_t { linear, step };

struct AnimationChannel {
  std::uint32_t node{no_asset_index};
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
  std::vector<std::uint32_t> roots;
};

struct ImportedScene {
  std::uint32_t default_scene{no_asset_index};
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

[[nodiscard]] std::expected<ImportedScene, std::string>
import_gltf(const std::filesystem::path &source);
[[nodiscard]] std::vector<std::byte>
encode_imported_scene(const ImportedScene &scene);
[[nodiscard]] std::expected<ImportedScene, std::string>
decode_imported_scene(std::span<const std::byte> encoded);

} // namespace gloom::assets
