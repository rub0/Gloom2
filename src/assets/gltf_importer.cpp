#include <gloom/assets/gltf_importer.hpp>
#include <gloom/assets/rig.hpp>
#include <gloom/assets/animation.hpp>

#if defined(GLOOM_GLTF_IMPORTER)
#include <gloom/assets/mesh_processing.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <simdjson.h>
#endif

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace gloom::assets {
namespace {

template <typename Surface> auto surface_fields(Surface& s) {
  return std::array{&s.specular_color[0], &s.specular_color[1], &s.specular_color[2],
                    &s.specular_factor, &s.normal_scale, &s.occlusion_strength,
                    &s.anisotropy_strength, &s.anisotropy_rotation, &s.uv_scroll[0],
                    &s.uv_scroll[1], &s.lava_wave, &s.alpha_cutoff};
}

constexpr std::size_t maximum_elements = 1'000'000;
#if !defined(GLOOM_GLTF_IMPORTER)
constexpr std::array<std::byte, 8> scene_magic{
    std::byte{'G'}, std::byte{'L'}, std::byte{'O'}, std::byte{'O'},
    std::byte{'M'}, std::byte{'S'}, std::byte{'C'}, std::byte{'N'}};
constexpr std::uint32_t scene_payload_version = 5;
constexpr std::size_t maximum_string_size = 1024 * 1024;

template <typename Integer>
void append_integer(std::vector<std::byte> &output, const Integer value) {
  static_assert(std::is_unsigned_v<Integer>);
  for (std::size_t index = 0; index < sizeof(Integer); ++index) {
    output.push_back(static_cast<std::byte>(value >> (index * 8)));
  }
}

void append_float(std::vector<std::byte> &output, const float value) {
  append_integer(output, std::bit_cast<std::uint32_t>(value));
}

void append_string(std::vector<std::byte> &output,
                   const std::string_view value) {
  if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw std::length_error{"Imported scene string is too large"};
  }
  append_integer(output, static_cast<std::uint32_t>(value.size()));
  output.insert(
      output.end(), reinterpret_cast<const std::byte *>(value.data()),
      reinterpret_cast<const std::byte *>(value.data() + value.size()));
}

class Reader final {
public:
  explicit Reader(const std::span<const std::byte> bytes) : bytes_{bytes} {}

  template <typename Integer> [[nodiscard]] std::optional<Integer> integer() {
    static_assert(std::is_unsigned_v<Integer>);
    if (remaining() < sizeof(Integer)) {
      return std::nullopt;
    }
    Integer value = 0;
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
      value |=
          static_cast<Integer>(std::to_integer<unsigned int>(bytes_[offset_++]))
          << (index * 8);
    }
    return value;
  }

  [[nodiscard]] std::optional<float> floating() {
    const auto bits = integer<std::uint32_t>();
    if (!bits) {
      return std::nullopt;
    }
    const float value = std::bit_cast<float>(*bits);
    return std::isfinite(value) ? std::optional<float>{value} : std::nullopt;
  }

  [[nodiscard]] std::optional<std::string> string() {
    const auto size = integer<std::uint32_t>();
    if (!size || *size > maximum_string_size || remaining() < *size) {
      return std::nullopt;
    }
    const auto *begin = reinterpret_cast<const char *>(bytes_.data() + offset_);
    std::string value{begin, begin + *size};
    offset_ += *size;
    return value;
  }

  [[nodiscard]] std::size_t remaining() const noexcept {
    return bytes_.size() - offset_;
  }

private:
  std::span<const std::byte> bytes_;
  std::size_t offset_{0};
};
#endif

#if defined(GLOOM_GLTF_IMPORTER)
[[nodiscard]] std::string fastgltf_error(const std::string_view operation,
                                         const fastgltf::Error error) {
  return std::string{operation} + ": " +
         std::string{fastgltf::getErrorMessage(error)};
}

[[nodiscard]] std::uint32_t optional_index(const auto &value) {
  if (!value) {
    return no_asset_index;
  }
  if (*value > std::numeric_limits<std::uint32_t>::max()) {
    throw std::length_error{"glTF index exceeds the cooked scene format"};
  }
  return static_cast<std::uint32_t>(*value);
}
#endif

[[nodiscard]] bool count_fits(const std::size_t count) noexcept {
  return count <= maximum_elements &&
         count <= std::numeric_limits<std::uint32_t>::max();
}

[[nodiscard]] bool validate_scene_indices(const ImportedScene &scene) {
  if (!valid_bind_rigs(scene) || !valid_animations(scene)) return false;
  if (scene.default_scene != no_asset_index &&
      scene.default_scene >= scene.scenes.size()) {
    return false;
  }
  for (const auto &primitive : scene.primitives) {
    if (primitive.material != no_asset_index &&
        primitive.material >= scene.materials.size()) {
      return false;
    }
    if (std::ranges::any_of(primitive.indices, [&](const std::uint32_t index) {
          return index >= primitive.vertices.size();
        })) {
      return false;
    }
    for (const auto &vertex : primitive.vertices) {
      if (!std::ranges::all_of(
              vertex.position,
              [](const float value) { return std::isfinite(value); }) ||
          !std::ranges::all_of(
              vertex.normal,
              [](const float value) { return std::isfinite(value); }) ||
          !std::ranges::all_of(
              vertex.texture_coordinate,
              [](const float value) { return std::isfinite(value); }) ||
          !std::ranges::all_of(vertex.texture_coordinate_1,
              [](const float value) { return std::isfinite(value); }) ||
          !std::ranges::all_of(
              vertex.tangent,
              [](const float value) { return std::isfinite(value); })) {
        return false;
      }
    }
    if (!std::ranges::all_of(primitive.bounds_center,
                             [](const float value) { return std::isfinite(value); }) ||
        !std::isfinite(primitive.bounds_radius) || primitive.bounds_radius < 0.0F ||
        primitive.lod_indices.size() > 2 ||
        std::ranges::any_of(primitive.lod_indices, [&](const auto& lod) {
          return lod.empty() || lod.size() % 3U != 0 ||
                 std::ranges::any_of(lod, [&](const std::uint32_t index) {
                   return index >= primitive.vertices.size();
                 });
        })) {
      return false;
    }
  }
  for (const auto &mesh : scene.meshes) {
    if (mesh.first_primitive > scene.primitives.size() ||
        mesh.primitive_count > scene.primitives.size() - mesh.first_primitive) {
      return false;
    }
  }
  for (const auto &material : scene.materials) {
    if (!std::ranges::all_of(
            material.base_color,
            [](const float value) { return std::isfinite(value); }) ||
        !std::ranges::all_of(
            material.emissive,
            [](const float value) { return std::isfinite(value); }) ||
        !std::isfinite(material.metallic) ||
        !std::isfinite(material.roughness) ||
        !std::isfinite(material.alpha_cutoff) || !render::valid_material_surface(material.surface)) {
      return false;
    }
    for (const std::uint32_t texture :
         material_textures(material)) {
      if (texture != no_asset_index && texture >= scene.textures.size()) {
        return false;
      }
    }
  }
  for (const auto &texture : scene.textures) {
    if (texture.image != no_asset_index &&
        texture.image >= scene.images.size()) {
      return false;
    }
  }
  for (const auto &node : scene.nodes) {
    if (!std::ranges::all_of(
            node.local_transform,
            [](const float value) { return std::isfinite(value); }) ||
        (node.mesh != no_asset_index && node.mesh >= scene.meshes.size()) ||
        std::ranges::any_of(node.children, [&](const std::uint32_t child) {
          return child >= scene.nodes.size();
        })) {
      return false;
    }
  }
  for (const auto &definition : scene.scenes) {
    if (std::ranges::any_of(definition.roots, [&](const std::uint32_t root) {
          return root >= scene.nodes.size();
        })) {
      return false;
    }
  }
  return true;
}

} // namespace

#if defined(GLOOM_GLTF_IMPORTER)
std::expected<ImportedScene, std::string>
import_gltf(const std::filesystem::path &source) {
  auto data = fastgltf::GltfDataBuffer::FromPath(source);
  if (data.error() != fastgltf::Error::None) {
    return std::unexpected{fastgltf_error("Could not read glTF", data.error())};
  }
  fastgltf::Parser parser{fastgltf::Extensions::KHR_materials_specular |
      fastgltf::Extensions::KHR_materials_anisotropy |
      fastgltf::Extensions::KHR_materials_emissive_strength |
      fastgltf::Extensions::KHR_texture_transform};
  struct Extra { render::MaterialSurface surface; std::array<std::uint32_t, 2> textures{no_asset_index,no_asset_index}; };
  struct Extras { std::vector<Extra> values; bool invalid{false}; } extras;
  parser.setUserPointer(&extras);
  parser.setExtrasParseCallback([](simdjson::dom::object* object, std::size_t index,
                                  fastgltf::Category category, void* user) {
    if (category != fastgltf::Category::Materials) return;
    auto& output = *static_cast<Extras*>(user);
    if (index >= maximum_elements) { output.invalid = true; return; }
    simdjson::dom::object properties;
    const auto error = (*object)["gloom"].get(properties);
    if (error == simdjson::NO_SUCH_FIELD) return;
    if (error) { output.invalid = true; return; }
    output.values.resize(std::max(output.values.size(), index+1));
    auto& value = output.values[index];
    for (const auto field : properties) {
      if (field.key == "sourceMaterial") continue;
      if (field.key == "uvScroll") {
        simdjson::dom::array array;
        if (field.value.get(array) || array.size()!=2) { output.invalid=true; continue; }
        std::size_t axis=0;
        for (const auto item : array) { double number;
          if (item.get(number)) { output.invalid=true; break; }
          value.surface.uv_scroll[axis++] = static_cast<float>(number);
        }
      } else if (field.key == "lavaWave") {
        double number;
        if (field.value.get(number)) output.invalid=true;
        else value.surface.lava_wave=static_cast<float>(number);
      } else if (field.key == "additive") {
        bool enabled;
        if (field.value.get(enabled)) output.invalid=true;
        else if (enabled) value.surface.alpha_mode=3;
      } else if (field.key == "detailTexture" || field.key == "lightmapTexture") {
        std::uint64_t texture;
        if (field.value.get(texture) || texture>=maximum_elements) output.invalid=true;
        else value.textures[field.key == "detailTexture" ? 0 : 1]=static_cast<std::uint32_t>(texture);
      } else output.invalid=true;
    }
  });
  auto parsed = parser.loadGltf(data.get(), source.parent_path(),
                                fastgltf::Options::LoadExternalBuffers |
                                    fastgltf::Options::GenerateMeshIndices);
  if (parsed.error() != fastgltf::Error::None) {
    return std::unexpected{
        fastgltf_error("Could not parse glTF", parsed.error())};
  }
  if (const auto validation = fastgltf::validate(parsed.get());
      validation != fastgltf::Error::None) {
    return std::unexpected{
        fastgltf_error("glTF validation failed", validation)};
  }
  if (extras.invalid) return std::unexpected{"Invalid Gloom material extras"};
  const auto &asset = parsed.get();
  if (asset.animations.size()>256) return std::unexpected{"Too many animation clips"};
  if (!count_fits(asset.meshes.size()) || !count_fits(asset.materials.size()) ||
      !count_fits(asset.textures.size()) || !count_fits(asset.images.size()) ||
      !count_fits(asset.nodes.size()) || !count_fits(asset.scenes.size()) || !count_fits(asset.skins.size())) {
    return std::unexpected{"glTF exceeds cooked scene element limits"};
  }

  ImportedScene scene;
  scene.default_scene = optional_index(asset.defaultScene);
  scene.materials.reserve(asset.materials.size());
  for (const auto &source_material : asset.materials) {
    ImportedMaterial material;
    const auto material_index = scene.materials.size();
    if (material_index < extras.values.size()) {
      material.surface = extras.values[material_index].surface;
      material.extra_textures[5] = extras.values[material_index].textures[0];
      material.extra_textures[6] = extras.values[material_index].textures[1];
      material.surface.mapping[9].uv_set = 1;
    }
    const auto mapping = [&](const auto& texture, std::size_t slot) {
      if (!texture) return;
      auto& map = material.surface.mapping[slot];
      map.uv_set = static_cast<std::uint32_t>(texture->texCoordIndex);
      if (texture->transform) {
        const auto& transform = *texture->transform;
        map.scale = {transform.uvScale[0],transform.uvScale[1]};
        map.offset = {transform.uvOffset[0],transform.uvOffset[1]};
        map.rotation = transform.rotation;
        if (transform.texCoordIndex) map.uv_set = static_cast<std::uint32_t>(*transform.texCoordIndex);
      }
    };
    material.name = source_material.name;
    for (std::size_t index = 0; index < 4; ++index) {
      material.base_color[index] =
          source_material.pbrData.baseColorFactor[index];
    }
    for (std::size_t index = 0; index < 3; ++index) {
      material.emissive[index] = source_material.emissiveFactor[index] * source_material.emissiveStrength;
    }
    material.metallic = source_material.pbrData.metallicFactor;
    material.roughness = source_material.pbrData.roughnessFactor;
    material.alpha_cutoff = source_material.alphaCutoff;
    material.alpha_mode = static_cast<std::uint8_t>(source_material.alphaMode);
    material.double_sided = source_material.doubleSided;
    if (source_material.pbrData.baseColorTexture) {
      material.base_color_texture = static_cast<std::uint32_t>(
          source_material.pbrData.baseColorTexture->textureIndex);
    }
    if (source_material.pbrData.metallicRoughnessTexture) {
      material.metallic_roughness_texture = static_cast<std::uint32_t>(
          source_material.pbrData.metallicRoughnessTexture->textureIndex);
    }
    if (source_material.normalTexture) {
      material.normal_texture = static_cast<std::uint32_t>(
          source_material.normalTexture->textureIndex);
    }
    if (material.surface.alpha_mode != 3) material.surface.alpha_mode = material.alpha_mode;
    material.surface.alpha_cutoff = material.alpha_cutoff;
    material.surface.double_sided = material.double_sided;
    mapping(source_material.pbrData.baseColorTexture, 0);
    mapping(source_material.pbrData.metallicRoughnessTexture, 1);
    mapping(source_material.normalTexture, 2);
    if (source_material.normalTexture) material.surface.normal_scale = source_material.normalTexture->scale;
    const auto extra_texture = [&](const auto& texture, std::size_t index) {
      if (texture) material.extra_textures[index] = static_cast<std::uint32_t>(texture->textureIndex);
      mapping(texture, index+3);
    };
    extra_texture(source_material.emissiveTexture, 0);
    extra_texture(source_material.occlusionTexture, 1);
    if (source_material.occlusionTexture) material.surface.occlusion_strength = source_material.occlusionTexture->strength;
    if (source_material.specular) {
      const auto& spec = *source_material.specular;
      material.surface.specular_factor = spec.specularFactor;
      material.surface.specular_color = {spec.specularColorFactor[0],spec.specularColorFactor[1],spec.specularColorFactor[2]};
      extra_texture(spec.specularTexture, 2);
      extra_texture(spec.specularColorTexture, 3);
    }
    if (source_material.anisotropy) {
      material.surface.anisotropy_strength = source_material.anisotropy->anisotropyStrength;
      material.surface.anisotropy_rotation = source_material.anisotropy->anisotropyRotation;
      extra_texture(source_material.anisotropy->anisotropyTexture, 4);
    }
    scene.materials.push_back(std::move(material));
  }

  scene.textures.reserve(asset.textures.size());
  for (const auto &source_texture : asset.textures) {
    scene.textures.push_back(
        {.image = optional_index(source_texture.imageIndex)});
  }
  scene.images.reserve(asset.images.size());
  for (const auto &source_image : asset.images) {
    ImportedImage image{.name = std::string{source_image.name}};
    if (const auto *uri =
            std::get_if<fastgltf::sources::URI>(&source_image.data)) {
      if (uri->uri.isDataUri()) {
        image.embedded = true;
      } else if (uri->uri.isLocalPath()) {
        image.external_uri = uri->uri.fspath().generic_string();
      } else {
        return std::unexpected{"glTF image uses a non-local URI"};
      }
    } else {
      image.embedded = true;
    }
    scene.images.push_back(std::move(image));
  }

  scene.meshes.reserve(asset.meshes.size());
  for (const auto &source_mesh : asset.meshes) {
    ImportedMesh mesh{
        .name = std::string{source_mesh.name},
        .first_primitive = static_cast<std::uint32_t>(scene.primitives.size()),
    };
    if (!count_fits(source_mesh.primitives.size()) ||
        scene.primitives.size() + source_mesh.primitives.size() >
            maximum_elements) {
      return std::unexpected{"glTF contains too many mesh primitives"};
    }
    for (const auto &source_primitive : source_mesh.primitives) {
      if (!source_primitive.targets.empty() ||
          source_primitive.dracoCompression) {
        return std::unexpected{"Morph targets and Draco-compressed primitives "
                               "are not supported yet"};
      }
      if (source_primitive.type != fastgltf::PrimitiveType::Triangles) {
        return std::unexpected{"Only triangle glTF primitives are supported"};
      }
      const auto position_attribute =
          source_primitive.findAttribute("POSITION");
      if (position_attribute == source_primitive.attributes.end()) {
        return std::unexpected{"glTF primitive has no POSITION attribute"};
      }
      const auto &position_accessor =
          asset.accessors[position_attribute->accessorIndex];
      if (!count_fits(position_accessor.count)) {
        return std::unexpected{"glTF primitive has too many vertices"};
      }
      ImportedPrimitive primitive;
      if (source_primitive.findAttribute("JOINTS_2")!=source_primitive.attributes.end() ||
          source_primitive.findAttribute("WEIGHTS_2")!=source_primitive.attributes.end())
        return std::unexpected{"More than eight skin influences are unsupported"};
      primitive.material = optional_index(source_primitive.materialIndex);
      primitive.vertices.resize(position_accessor.count);
      for (unsigned channel=0;channel<2;++channel) {
        const auto joints=source_primitive.findAttribute(channel==0 ? "JOINTS_0" : "JOINTS_1");
        const auto weights=source_primitive.findAttribute(channel==0 ? "WEIGHTS_0" : "WEIGHTS_1");
        if ((joints==source_primitive.attributes.end()) != (weights==source_primitive.attributes.end()))
          return std::unexpected{"JOINTS and WEIGHTS must be paired"};
        if (joints==source_primitive.attributes.end()) continue;
        const auto& ja=asset.accessors[joints->accessorIndex];
        const auto& wa=asset.accessors[weights->accessorIndex];
        if (ja.count!=primitive.vertices.size() || wa.count!=ja.count ||
            ja.type!=fastgltf::AccessorType::Vec4 || wa.type!=fastgltf::AccessorType::Vec4 ||
            (ja.componentType!=fastgltf::ComponentType::UnsignedByte && ja.componentType!=fastgltf::ComponentType::UnsignedShort))
          return std::unexpected{"Invalid skin attribute layout"};
        fastgltf::iterateAccessorWithIndex<fastgltf::math::uvec4>(asset,ja,[&](auto value,std::size_t i){
          for (std::size_t k=0;k<4;++k) primitive.vertices[i].joints[channel*4+k]=static_cast<std::uint16_t>(value[k]);
        });
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset,wa,[&](auto value,std::size_t i){
          for (std::size_t k=0;k<4;++k) primitive.vertices[i].weights[channel*4+k]=value[k];
        });
      }
      fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
          asset, position_accessor,
          [&](const fastgltf::math::fvec3 value, const std::size_t index) {
            primitive.vertices[index].position = {value[0], value[1], value[2]};
          });
      const auto normal_attribute = source_primitive.findAttribute("NORMAL");
      if (normal_attribute != source_primitive.attributes.end()) {
        const auto &accessor = asset.accessors[normal_attribute->accessorIndex];
        if (accessor.count != primitive.vertices.size()) {
          return std::unexpected{"glTF NORMAL count does not match POSITION"};
        }
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
            asset, accessor,
            [&](const fastgltf::math::fvec3 value, const std::size_t index) {
              primitive.vertices[index].normal = {value[0], value[1], value[2]};
            });
      }
      const auto texture_attribute =
          source_primitive.findAttribute("TEXCOORD_0");
      if (texture_attribute != source_primitive.attributes.end()) {
        const auto &accessor =
            asset.accessors[texture_attribute->accessorIndex];
        if (accessor.count != primitive.vertices.size()) {
          return std::unexpected{
              "glTF TEXCOORD_0 count does not match POSITION"};
        }
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
            asset, accessor,
            [&](const fastgltf::math::fvec2 value, const std::size_t index) {
              primitive.vertices[index].texture_coordinate = {value[0],
                                                              value[1]};
            });
      }
      const auto uv1 = source_primitive.findAttribute("TEXCOORD_1");
      if (uv1 != source_primitive.attributes.end()) {
        const auto& accessor = asset.accessors[uv1->accessorIndex];
        if (accessor.count != primitive.vertices.size()) return std::unexpected{"UV1 count mismatch"};
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, accessor,
            [&](const auto value, std::size_t index) { primitive.vertices[index].texture_coordinate_1 = {value[0],value[1]}; });
      }
      if (primitive.material != no_asset_index) {
        const auto& material=scene.materials[primitive.material];
        const auto textures=material_textures(material);
        for (std::size_t slot=0;slot<textures.size();++slot) {
          if (textures[slot]==no_asset_index) continue;
          if ((material.surface.mapping[slot].uv_set==1 && uv1==source_primitive.attributes.end()) ||
              (material.surface.mapping[slot].uv_set==0 && texture_attribute==source_primitive.attributes.end()))
            return std::unexpected{"Material references an absent UV channel"};
        }
      }
      if (!source_primitive.indicesAccessor) {
        return std::unexpected{"glTF index generation failed"};
      }
      const auto &index_accessor =
          asset.accessors[*source_primitive.indicesAccessor];
      if (!count_fits(index_accessor.count)) {
        return std::unexpected{"glTF primitive has too many indices"};
      }
      primitive.indices.resize(index_accessor.count);
      fastgltf::iterateAccessorWithIndex<std::uint32_t>(
          asset, index_accessor,
          [&](const std::uint32_t value, const std::size_t index) {
            primitive.indices[index] = value;
          });
      if (auto processed = process_imported_primitive(
              primitive,
              {.source_normals = normal_attribute != source_primitive.attributes.end(),
               .source_texture_coordinates =
                   texture_attribute != source_primitive.attributes.end()});
          !processed) {
        return std::unexpected{"Could not process glTF mesh: " + processed.error()};
      }
      scene.primitives.push_back(std::move(primitive));
    }
    mesh.primitive_count = static_cast<std::uint32_t>(scene.primitives.size() -
                                                      mesh.first_primitive);
    scene.meshes.push_back(std::move(mesh));
  }

  scene.nodes.reserve(asset.nodes.size());
  for (const auto &source_node : asset.nodes) {
    ImportedNode node{.name = std::string{source_node.name},
                      .mesh = optional_index(source_node.meshIndex),
                      .skin = optional_index(source_node.skinIndex)};
    const auto transform = fastgltf::getTransformMatrix(source_node);
    for (std::size_t column = 0; column < 4; ++column) {
      for (std::size_t row = 0; row < 4; ++row) {
        node.local_transform[column * 4 + row] = transform[column][row];
      }
    }
    node.children.reserve(source_node.children.size());
    for (const std::size_t child : source_node.children) {
      node.children.push_back(static_cast<std::uint32_t>(child));
    }
    scene.nodes.push_back(std::move(node));
  }
  scene.scenes.reserve(asset.scenes.size());
  for (const auto &source_scene : asset.scenes) {
    ImportedSceneDefinition definition{.name = std::string{source_scene.name}};
    definition.roots.reserve(source_scene.nodeIndices.size());
    for (const std::size_t root : source_scene.nodeIndices) {
      definition.roots.push_back(static_cast<std::uint32_t>(root));
    }
    scene.scenes.push_back(std::move(definition));
  }
  for (const auto& source_skin:asset.skins) {
    ImportedSkin skin{.name=std::string{source_skin.name}};
    for (auto j:source_skin.joints) skin.joints.push_back(static_cast<std::uint32_t>(j));
    if (skin.joints.empty() || skin.joints.size()>256 || !source_skin.inverseBindMatrices)
      return std::unexpected{"A bind-pose skin requires 1..256 joints and inverse bind matrices"};
    const auto& accessor=asset.accessors[*source_skin.inverseBindMatrices];
    if (accessor.count!=skin.joints.size() || accessor.type!=fastgltf::AccessorType::Mat4)
      return std::unexpected{"Invalid inverse bind matrix count or type"};
    skin.inverse_bind_matrices.resize(skin.joints.size());
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fmat4x4>(asset,accessor,[&](const auto& value,std::size_t i){
      for (std::size_t c=0;c<4;++c) for (std::size_t r=0;r<4;++r) skin.inverse_bind_matrices[i][c*4+r]=value[c][r];
    });
    scene.skins.push_back(std::move(skin));
  }
  std::size_t animation_keys=0;
  for (const auto& animation:asset.animations) {
    AnimationClip clip{.name=std::string{animation.name}};
    for (const auto& source_channel:animation.channels) {
      if (!source_channel.nodeIndex || source_channel.samplerIndex>=animation.samplers.size() ||
          source_channel.path==fastgltf::AnimationPath::Weights)
        return std::unexpected{"Unsupported animation target"};
      const auto& sampler=animation.samplers[source_channel.samplerIndex];
      if (sampler.interpolation!=fastgltf::AnimationInterpolation::Linear && sampler.interpolation!=fastgltf::AnimationInterpolation::Step)
        return std::unexpected{"Only LINEAR and STEP animation are supported; bake cubic curves offline"};
      if (!std::holds_alternative<fastgltf::TRS>(asset.nodes[*source_channel.nodeIndex].transform))
        return std::unexpected{"Animated nodes must use TRS"};
      const auto& input=asset.accessors[sampler.inputAccessor];
      const auto& output=asset.accessors[sampler.outputAccessor];
      const bool rotation=source_channel.path==fastgltf::AnimationPath::Rotation;
      animation_keys+=input.count;
      if (animation_keys>maximum_elements || input.count==0 || input.count!=output.count ||
          input.type!=fastgltf::AccessorType::Scalar || input.componentType!=fastgltf::ComponentType::Float ||
          output.type!=(rotation?fastgltf::AccessorType::Vec4:fastgltf::AccessorType::Vec3) ||
          output.componentType!=fastgltf::ComponentType::Float)
        return std::unexpected{"Invalid animation accessor layout"};
      AnimationChannel channel{.node=static_cast<std::uint32_t>(*source_channel.nodeIndex),
        .path=rotation?AnimationPath::rotation:source_channel.path==fastgltf::AnimationPath::Translation?AnimationPath::translation:AnimationPath::scale,
        .interpolation=sampler.interpolation==fastgltf::AnimationInterpolation::Step?AnimationInterpolation::step:AnimationInterpolation::linear};
      fastgltf::iterateAccessor<float>(asset,input,[&](float t){channel.times.push_back(t);});
      if (rotation) fastgltf::iterateAccessor<fastgltf::math::fvec4>(asset,output,[&](auto v){channel.values.push_back({v[0],v[1],v[2],v[3]});});
      else fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset,output,[&](auto v){channel.values.push_back({v[0],v[1],v[2],0});});
      clip.duration=std::max(clip.duration,channel.times.back());
      clip.channels.push_back(std::move(channel));
    }
    scene.animations.push_back(std::move(clip));
  }
  if (!validate_scene_indices(scene)) {
    return std::unexpected{"Imported glTF contains invalid cross references"};
  }
  return scene;
}
#else
std::vector<std::byte> encode_imported_scene(const ImportedScene &scene) {
  if (!validate_scene_indices(scene)) {
    throw std::invalid_argument{"Imported scene cross references are invalid"};
  }
  for (const std::size_t count :
       {scene.primitives.size(), scene.meshes.size(), scene.materials.size(),
        scene.textures.size(), scene.images.size(), scene.nodes.size(),
        scene.scenes.size(), scene.skins.size(), scene.animations.size()}) {
    if (!count_fits(count)) {
      throw std::length_error{"Imported scene exceeds element limits"};
    }
  }
  std::vector<std::byte> output;
  output.insert(output.end(), scene_magic.begin(), scene_magic.end());
  append_integer(output, scene_payload_version);
  append_integer(output, scene.default_scene);
  append_integer(output, static_cast<std::uint32_t>(scene.primitives.size()));
  append_integer(output, static_cast<std::uint32_t>(scene.meshes.size()));
  append_integer(output, static_cast<std::uint32_t>(scene.materials.size()));
  append_integer(output, static_cast<std::uint32_t>(scene.textures.size()));
  append_integer(output, static_cast<std::uint32_t>(scene.images.size()));
  append_integer(output, static_cast<std::uint32_t>(scene.nodes.size()));
  append_integer(output, static_cast<std::uint32_t>(scene.scenes.size()));
  append_integer(output, static_cast<std::uint32_t>(scene.skins.size()));
  append_integer(output, static_cast<std::uint32_t>(scene.animations.size()));
  for (const auto &primitive : scene.primitives) {
    append_integer(output, primitive.material);
    append_integer(output,
                   static_cast<std::uint32_t>(primitive.vertices.size()));
    append_integer(output,
                   static_cast<std::uint32_t>(primitive.indices.size()));
    for (const auto &vertex : primitive.vertices) {
      for (const float value : vertex.position)
        append_float(output, value);
      for (const float value : vertex.normal)
        append_float(output, value);
      for (const float value : vertex.texture_coordinate)
        append_float(output, value);
      for (const float value : vertex.tangent)
        append_float(output, value);
      for (const float value : vertex.texture_coordinate_1) append_float(output, value);
      for (auto value:vertex.joints) append_integer(output,value);
      for (float value:vertex.weights) append_float(output,value);
    }
    for (const std::uint32_t index : primitive.indices)
      append_integer(output, index);
    for (const float value : primitive.bounds_center)
      append_float(output, value);
    append_float(output, primitive.bounds_radius);
    append_integer(output, static_cast<std::uint32_t>(primitive.lod_indices.size()));
    for (const auto& lod : primitive.lod_indices) {
      append_integer(output, static_cast<std::uint32_t>(lod.size()));
      for (const auto index : lod)
        append_integer(output, index);
    }
  }
  for (const auto &mesh : scene.meshes) {
    append_string(output, mesh.name);
    append_integer(output, mesh.first_primitive);
    append_integer(output, mesh.primitive_count);
  }
  for (const auto &material : scene.materials) {
    append_string(output, material.name);
    for (const float value : material.base_color)
      append_float(output, value);
    for (const float value : material.emissive)
      append_float(output, value);
    append_float(output, material.metallic);
    append_float(output, material.roughness);
    append_float(output, material.alpha_cutoff);
    append_integer(output, material.alpha_mode);
    append_integer(output, static_cast<std::uint8_t>(material.double_sided));
    append_integer(output, std::uint16_t{0});
    append_integer(output, material.base_color_texture);
    append_integer(output, material.metallic_roughness_texture);
    append_integer(output, material.normal_texture);
    for (const auto field : surface_fields(material.surface)) append_float(output, *field);
    append_integer(output, material.surface.alpha_mode);
    append_integer(output, static_cast<std::uint8_t>(material.surface.double_sided));
    for (const auto& map : material.surface.mapping) {
      for (const auto value : {map.scale[0],map.scale[1],map.offset[0],map.offset[1],map.rotation}) append_float(output,value);
      append_integer(output,map.uv_set);
    }
    for (const auto texture : material.extra_textures) append_integer(output,texture);
  }
  for (const auto &texture : scene.textures)
    append_integer(output, texture.image);
  for (const auto &image : scene.images) {
    append_string(output, image.name);
    append_string(output, image.external_uri);
    append_integer(output, static_cast<std::uint8_t>(image.embedded));
  }
  for (const auto &node : scene.nodes) {
    append_string(output, node.name);
    for (const float value : node.local_transform)
      append_float(output, value);
    append_integer(output, node.mesh);
    append_integer(output, node.skin);
    append_integer(output, static_cast<std::uint32_t>(node.children.size()));
    for (const std::uint32_t child : node.children)
      append_integer(output, child);
  }
  for (const auto &definition : scene.scenes) {
    append_string(output, definition.name);
    append_integer(output, static_cast<std::uint32_t>(definition.roots.size()));
    for (const std::uint32_t root : definition.roots)
      append_integer(output, root);
  }
  for (const auto& skin:scene.skins) {
    append_string(output,skin.name);
    append_integer(output,static_cast<std::uint32_t>(skin.joints.size()));
    for (auto joint:skin.joints) append_integer(output,joint);
    for (const auto& matrix:skin.inverse_bind_matrices) for (float value:matrix) append_float(output,value);
  }
  for (const auto& clip:scene.animations) {
    append_string(output,clip.name);append_float(output,clip.duration);
    append_integer(output,static_cast<std::uint32_t>(clip.channels.size()));
    for (const auto& channel:clip.channels) {
      append_integer(output,channel.node);
      append_integer(output,static_cast<std::uint8_t>(channel.path));
      append_integer(output,static_cast<std::uint8_t>(channel.interpolation));
      append_integer(output,static_cast<std::uint32_t>(channel.times.size()));
      for (float t:channel.times) append_float(output,t);
      for (const auto& v:channel.values) for (float x:v) append_float(output,x);
    }
  }
  return output;
}

std::expected<ImportedScene, std::string>
decode_imported_scene(const std::span<const std::byte> encoded) {
  if (encoded.size() < scene_magic.size() ||
      !std::ranges::equal(scene_magic, encoded.first(scene_magic.size()))) {
    return std::unexpected{"Cooked scene payload header is invalid"};
  }
  Reader reader{encoded.subspan(scene_magic.size())};
  const auto version = reader.integer<std::uint32_t>();
  const auto default_scene = reader.integer<std::uint32_t>();
  std::array<std::uint32_t, 9> counts{};
  for (auto &count : counts) {
    const auto value = reader.integer<std::uint32_t>();
    if (!value || *value > maximum_elements) {
      return std::unexpected{"Cooked scene payload counts are invalid"};
    }
    count = *value;
  }
  if (!version || *version != scene_payload_version || !default_scene) {
    return std::unexpected{"Cooked scene payload version is unsupported"};
  }
  ImportedScene scene;
  scene.default_scene = *default_scene;
  scene.primitives.reserve(counts[0]);
  for (std::uint32_t primitive_index = 0; primitive_index < counts[0];
       ++primitive_index) {
    const auto material = reader.integer<std::uint32_t>();
    const auto vertex_count = reader.integer<std::uint32_t>();
    const auto index_count = reader.integer<std::uint32_t>();
    if (!material || !vertex_count || !index_count ||
        *vertex_count > maximum_elements || *index_count > maximum_elements ||
        static_cast<std::uint64_t>(*vertex_count) * (22U * sizeof(float)+8U*sizeof(std::uint16_t)) +
                static_cast<std::uint64_t>(*index_count) *
                    sizeof(std::uint32_t) >
            reader.remaining()) {
      return std::unexpected{"Cooked scene primitive header is invalid"};
    }
    ImportedPrimitive primitive{.material = *material};
    primitive.vertices.resize(*vertex_count);
    for (auto &vertex : primitive.vertices) {
      for (float &value : vertex.position) {
        const auto decoded = reader.floating();
        if (!decoded)
          return std::unexpected{"Cooked vertex is truncated"};
        value = *decoded;
      }
      for (float &value : vertex.normal) {
        const auto decoded = reader.floating();
        if (!decoded)
          return std::unexpected{"Cooked normal is truncated"};
        value = *decoded;
      }
      for (float &value : vertex.texture_coordinate) {
        const auto decoded = reader.floating();
        if (!decoded)
          return std::unexpected{"Cooked UV is truncated"};
        value = *decoded;
      }
      for (float &value : vertex.tangent) {
        const auto decoded = reader.floating();
        if (!decoded)
          return std::unexpected{"Cooked tangent is truncated"};
        value = *decoded;
      }
      for (auto& value : vertex.texture_coordinate_1) {
        const auto decoded = reader.floating();
        if (!decoded) return std::unexpected{"Cooked UV1 truncated"};
        value = *decoded;
      }
      for (auto& value:vertex.joints) {
        const auto decoded=reader.integer<std::uint16_t>();
        if (!decoded) return std::unexpected{"Cooked joints truncated"};value=*decoded;
      }
      for (auto& value:vertex.weights) {
        const auto decoded=reader.floating();
        if (!decoded) return std::unexpected{"Cooked weights truncated"};value=*decoded;
      }
    }
    primitive.indices.resize(*index_count);
    for (auto &index : primitive.indices) {
      const auto decoded = reader.integer<std::uint32_t>();
      if (!decoded)
        return std::unexpected{"Cooked indices are truncated"};
      index = *decoded;
    }
    for (float& value : primitive.bounds_center) {
      const auto decoded = reader.floating();
      if (!decoded)
        return std::unexpected{"Cooked mesh bounds are truncated"};
      value = *decoded;
    }
    const auto bounds_radius = reader.floating();
    const auto lod_count = reader.integer<std::uint32_t>();
    if (!bounds_radius || !lod_count || *bounds_radius < 0.0F || *lod_count > 2U)
      return std::unexpected{"Cooked mesh bounds or LOD count is invalid"};
    primitive.bounds_radius = *bounds_radius;
    primitive.lod_indices.reserve(*lod_count);
    for (std::uint32_t lod = 0; lod < *lod_count; ++lod) {
      const auto count = reader.integer<std::uint32_t>();
      if (!count || *count == 0 || *count > maximum_elements ||
          static_cast<std::uint64_t>(*count) * sizeof(std::uint32_t) > reader.remaining())
        return std::unexpected{"Cooked mesh LOD is invalid"};
      auto& indices = primitive.lod_indices.emplace_back(*count);
      for (auto& index : indices) {
        const auto decoded = reader.integer<std::uint32_t>();
        if (!decoded)
          return std::unexpected{"Cooked mesh LOD is truncated"};
        index = *decoded;
      }
    }
    scene.primitives.push_back(std::move(primitive));
  }
  scene.meshes.reserve(counts[1]);
  for (std::uint32_t index = 0; index < counts[1]; ++index) {
    const auto name = reader.string();
    const auto first = reader.integer<std::uint32_t>();
    const auto count = reader.integer<std::uint32_t>();
    if (!name || !first || !count)
      return std::unexpected{"Cooked mesh is truncated"};
    scene.meshes.push_back(
        {.name = *name, .first_primitive = *first, .primitive_count = *count});
  }
  scene.materials.reserve(counts[2]);
  for (std::uint32_t index = 0; index < counts[2]; ++index) {
    ImportedMaterial material;
    const auto name = reader.string();
    if (!name)
      return std::unexpected{"Cooked material is truncated"};
    material.name = *name;
    for (float &value : material.base_color) {
      const auto decoded = reader.floating();
      if (!decoded)
        return std::unexpected{"Cooked material is truncated"};
      value = *decoded;
    }
    for (float &value : material.emissive) {
      const auto decoded = reader.floating();
      if (!decoded)
        return std::unexpected{"Cooked material is truncated"};
      value = *decoded;
    }
    const auto metallic = reader.floating();
    const auto roughness = reader.floating();
    const auto cutoff = reader.floating();
    const auto alpha = reader.integer<std::uint8_t>();
    const auto sided = reader.integer<std::uint8_t>();
    const auto reserved = reader.integer<std::uint16_t>();
    const auto base = reader.integer<std::uint32_t>();
    const auto mr = reader.integer<std::uint32_t>();
    const auto normal = reader.integer<std::uint32_t>();
    if (!metallic || !roughness || !cutoff || !alpha || !sided || !reserved ||
        *reserved != 0 || !base || !mr || !normal || *sided > 1)
      return std::unexpected{"Cooked material fields are invalid"};
    material.metallic = *metallic;
    material.roughness = *roughness;
    material.alpha_cutoff = *cutoff;
    material.alpha_mode = *alpha;
    material.double_sided = *sided != 0;
    material.base_color_texture = *base;
    material.metallic_roughness_texture = *mr;
    material.normal_texture = *normal;
    for (auto field : surface_fields(material.surface)) {
      const auto decoded = reader.floating();
      if (!decoded) return std::unexpected{"Cooked material surface truncated"};
      *field = *decoded;
    }
    const auto mode=reader.integer<std::uint32_t>();
    const auto two_sided=reader.integer<std::uint8_t>();
    if (!mode || !two_sided || *two_sided>1) return std::unexpected{"Invalid surface flags"};
    material.surface.alpha_mode=*mode;
    material.surface.double_sided=*two_sided!=0;
    for (auto& map : material.surface.mapping) {
      for (auto field : {&map.scale[0],&map.scale[1],&map.offset[0],&map.offset[1],&map.rotation}) {
        const auto decoded=reader.floating();
        if (!decoded) return std::unexpected{"Cooked texture mapping truncated"};
        *field=*decoded;
      }
      const auto uv=reader.integer<std::uint32_t>();
      if (!uv) return std::unexpected{"Cooked UV selector truncated"};
      map.uv_set=*uv;
    }
    for (auto& texture : material.extra_textures) {
      const auto decoded=reader.integer<std::uint32_t>();
      if (!decoded) return std::unexpected{"Cooked extra texture truncated"};
      texture=*decoded;
    }
    scene.materials.push_back(std::move(material));
  }
  scene.textures.reserve(counts[3]);
  for (std::uint32_t index = 0; index < counts[3]; ++index) {
    const auto image = reader.integer<std::uint32_t>();
    if (!image)
      return std::unexpected{"Cooked texture is truncated"};
    scene.textures.push_back({.image = *image});
  }
  scene.images.reserve(counts[4]);
  for (std::uint32_t index = 0; index < counts[4]; ++index) {
    const auto name = reader.string();
    const auto uri = reader.string();
    const auto embedded = reader.integer<std::uint8_t>();
    if (!name || !uri || !embedded || *embedded > 1)
      return std::unexpected{"Cooked image is invalid"};
    scene.images.push_back(
        {.name = *name, .external_uri = *uri, .embedded = *embedded != 0});
  }
  scene.nodes.reserve(counts[5]);
  for (std::uint32_t index = 0; index < counts[5]; ++index) {
    ImportedNode node;
    const auto name = reader.string();
    if (!name)
      return std::unexpected{"Cooked node is truncated"};
    node.name = *name;
    for (float &value : node.local_transform) {
      const auto decoded = reader.floating();
      if (!decoded)
        return std::unexpected{"Cooked node transform is truncated"};
      value = *decoded;
    }
    const auto mesh = reader.integer<std::uint32_t>();
    const auto skin = reader.integer<std::uint32_t>();
    const auto child_count = reader.integer<std::uint32_t>();
    if (!mesh || !skin || !child_count || *child_count > maximum_elements ||
        static_cast<std::uint64_t>(*child_count) * sizeof(std::uint32_t) >
            reader.remaining())
      return std::unexpected{"Cooked node fields are invalid"};
    node.mesh = *mesh;
    node.skin = *skin;
    node.children.resize(*child_count);
    for (auto &child : node.children) {
      const auto decoded = reader.integer<std::uint32_t>();
      if (!decoded)
        return std::unexpected{"Cooked node children are truncated"};
      child = *decoded;
    }
    scene.nodes.push_back(std::move(node));
  }
  scene.scenes.reserve(counts[6]);
  for (std::uint32_t index = 0; index < counts[6]; ++index) {
    ImportedSceneDefinition definition;
    const auto name = reader.string();
    const auto root_count = reader.integer<std::uint32_t>();
    if (!name || !root_count || *root_count > maximum_elements ||
        static_cast<std::uint64_t>(*root_count) * sizeof(std::uint32_t) >
            reader.remaining())
      return std::unexpected{"Cooked scene definition is invalid"};
    definition.name = *name;
    definition.roots.resize(*root_count);
    for (auto &root : definition.roots) {
      const auto decoded = reader.integer<std::uint32_t>();
      if (!decoded)
        return std::unexpected{"Cooked scene roots are truncated"};
      root = *decoded;
    }
    scene.scenes.push_back(std::move(definition));
  }
  for (std::uint32_t i=0;i<counts[7];++i) {
    const auto name=reader.string();const auto count=reader.integer<std::uint32_t>();
    if (!name || !count || *count==0 || *count>256 || reader.remaining()<*count*68ULL)
      return std::unexpected{"Invalid cooked skin header"};
    ImportedSkin skin{.name=*name};
    for (std::uint32_t j=0;j<*count;++j) skin.joints.push_back(*reader.integer<std::uint32_t>());
    skin.inverse_bind_matrices.resize(*count);
    for (auto& matrix:skin.inverse_bind_matrices) for (auto& value:matrix) {
      const auto decoded=reader.floating();if (!decoded) return std::unexpected{"Invalid inverse bind matrix"};value=*decoded;
    }
    scene.skins.push_back(std::move(skin));
  }
  std::size_t animation_keys=0;
  if (counts[8]>256) return std::unexpected{"Too many cooked clips"};
  for (std::uint32_t i=0;i<counts[8];++i) {
    const auto name=reader.string();const auto duration=reader.floating();const auto count=reader.integer<std::uint32_t>();
    if (!name || !duration || !count || *count>scene.nodes.size()*3 || reader.remaining()<*count*10ULL)
      return std::unexpected{"Invalid cooked animation header"};
    AnimationClip clip{.name=*name,.duration=*duration};
    for (std::uint32_t c=0;c<*count;++c) {
      const auto node=reader.integer<std::uint32_t>();const auto path=reader.integer<std::uint8_t>();
      const auto mode=reader.integer<std::uint8_t>();const auto keys=reader.integer<std::uint32_t>();
      if (!node || !path || !mode || !keys || *keys>maximum_elements || reader.remaining()<*keys*20ULL)
        return std::unexpected{"Invalid cooked animation channel"};
      animation_keys+=*keys;if (animation_keys>maximum_elements) return std::unexpected{"Cooked animation key budget exceeded"};
      AnimationChannel channel{.node=*node,.path=static_cast<AnimationPath>(*path),.interpolation=static_cast<AnimationInterpolation>(*mode)};
      for (std::uint32_t k=0;k<*keys;++k) {
        const auto t=reader.floating();if (!t) return std::unexpected{"Invalid animation time"};channel.times.push_back(*t);
      }
      channel.values.resize(*keys);
      for (auto& v:channel.values) for (auto& x:v) {
        const auto f=reader.floating();if (!f) return std::unexpected{"Invalid animation value"};x=*f;
      }
      clip.channels.push_back(std::move(channel));
    }
    scene.animations.push_back(std::move(clip));
  }
  if (reader.remaining() != 0 || !validate_scene_indices(scene)) {
    return std::unexpected{"Cooked scene payload has trailing or invalid data"};
  }
  return scene;
}
#endif

} // namespace gloom::assets
