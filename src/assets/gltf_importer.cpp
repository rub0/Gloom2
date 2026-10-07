#include <gloom/assets/gltf_importer.hpp>
#include <gloom/assets/rig.hpp>
#include <gloom/assets/animation.hpp>
#include <gloom/core/allocation_profile.hpp>
#include <math.h>
#include <string.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#if defined(GLOOM_GLTF_IMPORTER)
#include <gloom/assets/mesh_processing.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <simdjson.h>
#endif

#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <optional>

#include <string_view>
#include <type_traits>
#include <utility>

namespace gloom::assets {
namespace {

template <typename Surface> auto surface_fields(Surface& s) {
    return std::array{&s.specular_color[0], &s.specular_color[1], &s.specular_color[2], &s.specular_factor, &s.normal_scale, &s.occlusion_strength,
        &s.anisotropy_strength, &s.anisotropy_rotation, &s.uv_scroll[0], &s.uv_scroll[1], &s.lava_wave, &s.alpha_cutoff};
}

constexpr size_t maximum_elements = 1'000'000;
constexpr size_t maximum_string_size = 1024 * 1024;
#if !defined(GLOOM_GLTF_IMPORTER)
constexpr std::array<std::byte, 8> scene_magic{
    std::byte{'G'}, std::byte{'L'}, std::byte{'O'}, std::byte{'O'}, std::byte{'M'}, std::byte{'S'}, std::byte{'C'}, std::byte{'N'}};
constexpr uint32 scene_payload_version = 5;

size_t encoded_scene_size(const ImportedScene& scene) {
    size_t size = 52 + scene.textures.size() * 4;
    for (const ImportedPrimitive& primitive : scene.primitives) {
        size += 32 + primitive.vertices.size() * 104 + primitive.indices.size() * 4;
        for (const std::vector<uint32>& lod : primitive.lod_indices)
            size += 4 + lod.size() * 4;
    }
    for (const ImportedMesh& mesh : scene.meshes)
        size += 12 + mesh.name.size();
    for (const ImportedMaterial& material : scene.materials)
        size += 141 + render::material_texture_count * 24 + material.name.size();
    for (const ImportedImage& image : scene.images)
        size += 9 + image.name.size() + image.external_uri.size();
    for (const ImportedNode& node : scene.nodes)
        size += 80 + node.name.size() + node.children.size() * 4;
    for (const ImportedSceneDefinition& definition : scene.scenes)
        size += 8 + definition.name.size() + definition.roots.size() * 4;
    for (const ImportedSkin& skin : scene.skins)
        size += 8 + skin.name.size() + skin.joints.size() * 68;
    for (const AnimationClip& clip : scene.animations) {
        size += 12 + clip.name.size();
        for (const AnimationChannel& channel : clip.channels)
            size += 10 + channel.times.size() * 20;
    }
    return size;
}

template <typename Integer> void append_integer(std::vector<std::byte>& output, const Integer value) {
    static_assert(std::is_unsigned_v<Integer>);
    for (size_t index = 0; index < sizeof(Integer); ++index) {
        output.push_back(static_cast<std::byte>(value >> (index * 8)));
    }
}

void append_float(std::vector<std::byte>& output, const float value) {
    append_integer(output, std::bit_cast<uint32>(value));
}

void append_string(std::vector<std::byte>& output, const std::string_view value) {
    assert(value.size() <= maximum_string_size);
    append_integer(output, static_cast<uint32>(value.size()));
    output.insert(output.end(), reinterpret_cast<const std::byte*>(value.data()), reinterpret_cast<const std::byte*>(value.data() + value.size()));
}

class Reader final {
  public:
    explicit Reader(const Span<const std::byte> bytes) : bytes_{bytes} {}

    template <typename Integer> [[nodiscard]] std::optional<Integer> integer() {
        static_assert(std::is_unsigned_v<Integer>);
        if (remaining() < sizeof(Integer)) {
            return std::nullopt;
        }
        Integer value = 0;
        for (size_t index = 0; index < sizeof(Integer); ++index) {
            value |= static_cast<Integer>(std::to_integer<unsigned int>(bytes_[offset_++])) << (index * 8);
        }
        return value;
    }

    [[nodiscard]] std::optional<float> floating() {
        const std::optional<uint32> bits = integer<uint32>();
        if (!bits) {
            return std::nullopt;
        }
        const float value = std::bit_cast<float>(*bits);
        return std::isfinite(value) ? std::optional<float>{value} : std::nullopt;
    }

    [[nodiscard]] std::optional<std::string> string() {
        const std::optional<uint32> size = integer<uint32>();
        if (!size || *size > maximum_string_size || remaining() < *size) {
            return std::nullopt;
        }
        const char* begin = reinterpret_cast<const char*>(bytes_.data() + offset_);
        std::string value{begin, begin + *size};
        offset_ += *size;
        return value;
    }

    [[nodiscard]] size_t remaining() const noexcept {
        return bytes_.size() - offset_;
    }

  private:
    Span<const std::byte> bytes_;
    size_t offset_{0};
};
#endif

#if defined(GLOOM_GLTF_IMPORTER)
[[nodiscard]] std::string fastgltf_error(const std::string_view operation, const fastgltf::Error error) {
    return std::string{operation} + ": " + std::string{fastgltf::getErrorMessage(error)};
}

struct Extra {
    render::MaterialSurface surface;
    std::array<uint32, 2> textures{no_asset_index, no_asset_index};
};
struct Extras {
    std::vector<Extra> values;
    bool invalid{false};
};

void parse_gloom_extras(simdjson::dom::object* object, size_t index, fastgltf::Category category, void* user) {
    if (category != fastgltf::Category::Materials)
        return;
    Extras& output = *static_cast<Extras*>(user);
    if (index >= maximum_elements) {
        output.invalid = true;
        return;
    }
    simdjson::dom::object properties;
    const simdjson::error_code error = (*object)["gloom"].get(properties);
    if (error == simdjson::NO_SUCH_FIELD)
        return;
    if (error) {
        output.invalid = true;
        return;
    }
    if (output.values.size() <= index)
        output.values.resize(index + 1);
    Extra& value = output.values[index];
    for (const simdjson::dom::key_value_pair field : properties) {
        if (field.key == "sourceMaterial")
            continue;
        if (field.key == "uvScroll") {
            simdjson::dom::array array;
            if (field.value.get(array) || array.size() != 2) {
                output.invalid = true;
                continue;
            }
            size_t axis = 0;
            for (const simdjson::dom::element item : array) {
                double number;
                if (item.get(number)) {
                    output.invalid = true;
                    break;
                }
                value.surface.uv_scroll[axis++] = static_cast<float>(number);
            }
        } else if (field.key == "lavaWave") {
            double number;
            if (field.value.get(number))
                output.invalid = true;
            else
                value.surface.lava_wave = static_cast<float>(number);
        } else if (field.key == "additive") {
            bool enabled;
            if (field.value.get(enabled))
                output.invalid = true;
            else if (enabled)
                value.surface.alpha_mode = 3;
        } else if (field.key == "detailTexture" || field.key == "lightmapTexture") {
            uint64 texture;
            if (field.value.get(texture) || texture >= maximum_elements)
                output.invalid = true;
            else
                value.textures[field.key == "detailTexture" ? 0 : 1] = static_cast<uint32>(texture);
        } else
            output.invalid = true;
    }
}
void import_mapping(render::TextureMapping& map, const fastgltf::TextureInfo* texture) {
    if (!texture)
        return;
    map.uv_set = static_cast<uint32>(texture->texCoordIndex);
    if (texture->transform) {
        const fastgltf::TextureTransform& transform = *texture->transform;
        map.scale = {transform.uvScale[0], transform.uvScale[1]};
        map.offset = {transform.uvOffset[0], transform.uvOffset[1]};
        map.rotation = transform.rotation;
        if (transform.texCoordIndex)
            map.uv_set = static_cast<uint32>(*transform.texCoordIndex);
    }
}
void import_extra(ImportedMaterial& material, const fastgltf::TextureInfo* texture, size_t index) {
    if (texture)
        material.extra_textures[index] = static_cast<uint32>(texture->textureIndex);
    import_mapping(material.surface.mapping[index + 3], texture);
}
[[nodiscard]] uint32 optional_index(fastgltf::Optional<size_t> value) {
    if (!value) {
        return no_asset_index;
    }
    assert(*value <= std::numeric_limits<uint32>::max());
    return static_cast<uint32>(*value);
}
#endif

[[nodiscard]] bool count_fits(const size_t count) noexcept {
    return count <= maximum_elements && count <= std::numeric_limits<uint32>::max();
}

#if !defined(GLOOM_GLTF_IMPORTER)
bool finite_values(Span<const float> values) {
    for (float value : values)
        if (!isfinite(value))
            return false;
    return true;
}
#endif
} // namespace

#if !defined(GLOOM_GLTF_IMPORTER)
bool validate_imported_scene(const ImportedScene& scene) {
    for (size_t count : {scene.primitives.size(), scene.meshes.size(), scene.materials.size(), scene.textures.size(), scene.images.size(), scene.nodes.size(),
             scene.scenes.size(), scene.skins.size(), scene.animations.size()})
        if (!count_fits(count))
            return false;
    if (!valid_bind_rigs(scene) || !valid_animations(scene) || (scene.default_scene != no_asset_index && scene.default_scene >= scene.scenes.size()))
        return false;
    for (const ImportedPrimitive& primitive : scene.primitives) {
        if (!count_fits(primitive.vertices.size()) || !count_fits(primitive.indices.size()) ||
            (primitive.material != no_asset_index && primitive.material >= scene.materials.size()))
            return false;
        for (uint32 index : primitive.indices)
            if (index >= primitive.vertices.size())
                return false;
        for (const ImportedVertex& vertex : primitive.vertices)
            if (!finite_values(vertex.position) || !finite_values(vertex.normal) || !finite_values(vertex.texture_coordinate) ||
                !finite_values(vertex.texture_coordinate_1) || !finite_values(vertex.tangent))
                return false;
        if (!finite_values(primitive.bounds_center) || !isfinite(primitive.bounds_radius) || primitive.bounds_radius < 0 || primitive.lod_indices.size() > 2)
            return false;
        for (const std::vector<uint32>& lod : primitive.lod_indices) {
            if (lod.empty() || lod.size() % 3 || !count_fits(lod.size()))
                return false;
            for (uint32 index : lod)
                if (index >= primitive.vertices.size())
                    return false;
        }
    }
    for (const ImportedMesh& mesh : scene.meshes)
        if (mesh.name.size() > maximum_string_size || mesh.first_primitive > scene.primitives.size() ||
            mesh.primitive_count > scene.primitives.size() - mesh.first_primitive)
            return false;
    for (const ImportedMaterial& material : scene.materials) {
        if (material.name.size() > maximum_string_size || !finite_values(material.base_color) || !finite_values(material.emissive) ||
            !isfinite(material.metallic) || !isfinite(material.roughness) || !isfinite(material.alpha_cutoff) ||
            !render::valid_material_surface(material.surface))
            return false;
        for (uint32 texture : material_textures(material))
            if (texture != no_asset_index && texture >= scene.textures.size())
                return false;
    }
    for (const ImportedTexture& texture : scene.textures)
        if (texture.image != no_asset_index && texture.image >= scene.images.size())
            return false;
    for (const ImportedImage& image : scene.images)
        if (image.name.size() > maximum_string_size || image.external_uri.size() > maximum_string_size)
            return false;
    for (const ImportedNode& node : scene.nodes) {
        if (node.name.size() > maximum_string_size || !finite_values({node.local_transform.data(), 16}) || !count_fits(node.children.size()) ||
            (node.mesh != no_asset_index && node.mesh >= scene.meshes.size()))
            return false;
        for (uint32 child : node.children)
            if (child >= scene.nodes.size())
                return false;
    }
    for (const ImportedSceneDefinition& definition : scene.scenes) {
        if (definition.name.size() > maximum_string_size || !count_fits(definition.roots.size()))
            return false;
        for (uint32 root : definition.roots)
            if (root >= scene.nodes.size())
                return false;
    }
    for (const ImportedSkin& skin : scene.skins)
        if (skin.name.size() > maximum_string_size)
            return false;
    for (const AnimationClip& clip : scene.animations)
        if (clip.name.size() > maximum_string_size)
            return false;
    return true;
}
#endif

#if defined(GLOOM_GLTF_IMPORTER)
std::expected<ImportedScene, std::string> import_gltf(const std::filesystem::path& source) {
    fastgltf::Expected<fastgltf::GltfDataBuffer> data = fastgltf::GltfDataBuffer::FromPath(source);
    if (data.error() != fastgltf::Error::None) {
        return std::unexpected{fastgltf_error("Could not read glTF", data.error())};
    }
    fastgltf::Parser parser{fastgltf::Extensions::KHR_materials_specular | fastgltf::Extensions::KHR_materials_anisotropy |
                            fastgltf::Extensions::KHR_materials_emissive_strength | fastgltf::Extensions::KHR_texture_transform};
    Extras extras;
    parser.setUserPointer(&extras);
    parser.setExtrasParseCallback(parse_gloom_extras);
    fastgltf::Expected<fastgltf::Asset> parsed =
        parser.loadGltf(data.get(), source.parent_path(), fastgltf::Options::LoadExternalBuffers | fastgltf::Options::GenerateMeshIndices);
    if (parsed.error() != fastgltf::Error::None) {
        return std::unexpected{fastgltf_error("Could not parse glTF", parsed.error())};
    }
    if (const fastgltf::Error validation = fastgltf::validate(parsed.get()); validation != fastgltf::Error::None) {
        return std::unexpected{fastgltf_error("glTF validation failed", validation)};
    }
    if (extras.invalid)
        return std::unexpected{"Invalid Gloom material extras"};
    const fastgltf::Asset& asset = parsed.get();
    if (asset.animations.size() > 256)
        return std::unexpected{"Too many animation clips"};
    if (!count_fits(asset.meshes.size()) || !count_fits(asset.materials.size()) || !count_fits(asset.textures.size()) || !count_fits(asset.images.size()) ||
        !count_fits(asset.nodes.size()) || !count_fits(asset.scenes.size()) || !count_fits(asset.skins.size())) {
        return std::unexpected{"glTF exceeds cooked scene element limits"};
    }

    ImportedScene scene;
    scene.default_scene = optional_index(asset.defaultScene);
    scene.materials.reserve(asset.materials.size());
    for (const fastgltf::Material& source_material : asset.materials) {
        ImportedMaterial material;
        const size_t material_index = scene.materials.size();
        if (material_index < extras.values.size()) {
            material.surface = extras.values[material_index].surface;
            material.extra_textures[5] = extras.values[material_index].textures[0];
            material.extra_textures[6] = extras.values[material_index].textures[1];
            material.surface.mapping[9].uv_set = 1;
        }
        material.name = source_material.name;
        for (size_t index = 0; index < 4; ++index) {
            material.base_color[index] = source_material.pbrData.baseColorFactor[index];
        }
        for (size_t index = 0; index < 3; ++index) {
            material.emissive[index] = source_material.emissiveFactor[index] * source_material.emissiveStrength;
        }
        material.metallic = source_material.pbrData.metallicFactor;
        material.roughness = source_material.pbrData.roughnessFactor;
        material.alpha_cutoff = source_material.alphaCutoff;
        material.alpha_mode = static_cast<uint8>(source_material.alphaMode);
        material.double_sided = source_material.doubleSided;
        if (source_material.pbrData.baseColorTexture) {
            material.base_color_texture = static_cast<uint32>(source_material.pbrData.baseColorTexture->textureIndex);
        }
        if (source_material.pbrData.metallicRoughnessTexture) {
            material.metallic_roughness_texture = static_cast<uint32>(source_material.pbrData.metallicRoughnessTexture->textureIndex);
        }
        if (source_material.normalTexture) {
            material.normal_texture = static_cast<uint32>(source_material.normalTexture->textureIndex);
        }
        if (material.surface.alpha_mode != 3)
            material.surface.alpha_mode = material.alpha_mode;
        material.surface.alpha_cutoff = material.alpha_cutoff;
        material.surface.double_sided = material.double_sided;
        import_mapping(material.surface.mapping[0], source_material.pbrData.baseColorTexture ? &*source_material.pbrData.baseColorTexture : nullptr);
        import_mapping(
            material.surface.mapping[1], source_material.pbrData.metallicRoughnessTexture ? &*source_material.pbrData.metallicRoughnessTexture : nullptr);
        import_mapping(material.surface.mapping[2], source_material.normalTexture ? &*source_material.normalTexture : nullptr);
        if (source_material.normalTexture)
            material.surface.normal_scale = source_material.normalTexture->scale;
        import_extra(material, source_material.emissiveTexture ? &*source_material.emissiveTexture : nullptr, 0);
        import_extra(material, source_material.occlusionTexture ? &*source_material.occlusionTexture : nullptr, 1);
        if (source_material.occlusionTexture)
            material.surface.occlusion_strength = source_material.occlusionTexture->strength;
        if (source_material.specular) {
            const fastgltf::MaterialSpecular& spec = *source_material.specular;
            material.surface.specular_factor = spec.specularFactor;
            material.surface.specular_color = {spec.specularColorFactor[0], spec.specularColorFactor[1], spec.specularColorFactor[2]};
            import_extra(material, spec.specularTexture ? &*spec.specularTexture : nullptr, 2);
            import_extra(material, spec.specularColorTexture ? &*spec.specularColorTexture : nullptr, 3);
        }
        if (source_material.anisotropy) {
            material.surface.anisotropy_strength = source_material.anisotropy->anisotropyStrength;
            material.surface.anisotropy_rotation = source_material.anisotropy->anisotropyRotation;
            import_extra(material, source_material.anisotropy->anisotropyTexture ? &*source_material.anisotropy->anisotropyTexture : nullptr, 4);
        }
        scene.materials.push_back(std::move(material));
    }

    scene.textures.reserve(asset.textures.size());
    for (const fastgltf::Texture& source_texture : asset.textures) {
        scene.textures.push_back({.image = optional_index(source_texture.imageIndex)});
    }
    scene.images.reserve(asset.images.size());
    for (const fastgltf::Image& source_image : asset.images) {
        ImportedImage image{.name = std::string{source_image.name}};
        if (const fastgltf::sources::URI* uri = std::get_if<fastgltf::sources::URI>(&source_image.data)) {
            if (uri->uri.isDataUri()) {
                image.embedded = true;
            } else if (uri->uri.isLocalPath()) {
                image.external_uri = uri->uri.path();
            } else {
                return std::unexpected{"glTF image uses a non-local URI"};
            }
        } else {
            image.embedded = true;
        }
        scene.images.push_back(std::move(image));
    }

    scene.meshes.reserve(asset.meshes.size());
    for (const fastgltf::Mesh& source_mesh : asset.meshes) {
        ImportedMesh mesh{
            .name = std::string{source_mesh.name},
            .first_primitive = static_cast<uint32>(scene.primitives.size()),
        };
        if (!count_fits(source_mesh.primitives.size()) || scene.primitives.size() + source_mesh.primitives.size() > maximum_elements) {
            return std::unexpected{"glTF contains too many mesh primitives"};
        }
        for (const fastgltf::Primitive& source_primitive : source_mesh.primitives) {
            if (!source_primitive.targets.empty() || source_primitive.dracoCompression) {
                return std::unexpected{"Morph targets and Draco-compressed primitives "
                                       "are not supported yet"};
            }
            if (source_primitive.type != fastgltf::PrimitiveType::Triangles) {
                return std::unexpected{"Only triangle glTF primitives are supported"};
            }
            const fastgltf::Attribute* position_attribute = source_primitive.findAttribute("POSITION");
            if (position_attribute == source_primitive.attributes.end()) {
                return std::unexpected{"glTF primitive has no POSITION attribute"};
            }
            const fastgltf::Accessor& position_accessor = asset.accessors[position_attribute->accessorIndex];
            if (!count_fits(position_accessor.count) || position_accessor.type != fastgltf::AccessorType::Vec3) {
                return std::unexpected{"glTF primitive has too many vertices"};
            }
            ImportedPrimitive primitive;
            if (source_primitive.findAttribute("JOINTS_2") != source_primitive.attributes.end() ||
                source_primitive.findAttribute("WEIGHTS_2") != source_primitive.attributes.end())
                return std::unexpected{"More than eight skin influences are unsupported"};
            primitive.material = optional_index(source_primitive.materialIndex);
            primitive.vertices.resize(position_accessor.count);
            for (unsigned channel = 0; channel < 2; ++channel) {
                const fastgltf::Attribute* joints = source_primitive.findAttribute(channel == 0 ? "JOINTS_0" : "JOINTS_1");
                const fastgltf::Attribute* weights = source_primitive.findAttribute(channel == 0 ? "WEIGHTS_0" : "WEIGHTS_1");
                if ((joints == source_primitive.attributes.end()) != (weights == source_primitive.attributes.end()))
                    return std::unexpected{"JOINTS and WEIGHTS must be paired"};
                if (joints == source_primitive.attributes.end())
                    continue;
                const fastgltf::Accessor& ja = asset.accessors[joints->accessorIndex];
                const fastgltf::Accessor& wa = asset.accessors[weights->accessorIndex];
                if (ja.count != primitive.vertices.size() || wa.count != ja.count || ja.type != fastgltf::AccessorType::Vec4 ||
                    wa.type != fastgltf::AccessorType::Vec4 ||
                    (ja.componentType != fastgltf::ComponentType::UnsignedByte && ja.componentType != fastgltf::ComponentType::UnsignedShort))
                    return std::unexpected{"Invalid skin attribute layout"};
                size_t vertex_index = 0;
                for (fastgltf::math::uvec4 value : fastgltf::iterateAccessor<fastgltf::math::uvec4>(asset, ja)) {
                    for (size_t k = 0; k < 4; ++k)
                        primitive.vertices[vertex_index].joints[channel * 4 + k] = static_cast<uint16>(value[k]);
                    ++vertex_index;
                }
                vertex_index = 0;
                for (fastgltf::math::fvec4 value : fastgltf::iterateAccessor<fastgltf::math::fvec4>(asset, wa)) {
                    for (size_t k = 0; k < 4; ++k)
                        primitive.vertices[vertex_index].weights[channel * 4 + k] = value[k];
                    ++vertex_index;
                }
            }
            size_t vertex_index = 0;
            for (fastgltf::math::fvec3 value : fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, position_accessor))
                primitive.vertices[vertex_index++].position = {value[0], value[1], value[2]};
            const fastgltf::Attribute* normal_attribute = source_primitive.findAttribute("NORMAL");
            if (normal_attribute != source_primitive.attributes.end()) {
                const fastgltf::Accessor& accessor = asset.accessors[normal_attribute->accessorIndex];
                if (accessor.count != primitive.vertices.size() || accessor.type != fastgltf::AccessorType::Vec3) {
                    return std::unexpected{"glTF NORMAL count does not match POSITION"};
                }
                vertex_index = 0;
                for (fastgltf::math::fvec3 value : fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, accessor))
                    primitive.vertices[vertex_index++].normal = {value[0], value[1], value[2]};
            }
            const fastgltf::Attribute* texture_attribute = source_primitive.findAttribute("TEXCOORD_0");
            if (texture_attribute != source_primitive.attributes.end()) {
                const fastgltf::Accessor& accessor = asset.accessors[texture_attribute->accessorIndex];
                if (accessor.count != primitive.vertices.size() || accessor.type != fastgltf::AccessorType::Vec2) {
                    return std::unexpected{"glTF TEXCOORD_0 count does not match POSITION"};
                }
                vertex_index = 0;
                for (fastgltf::math::fvec2 value : fastgltf::iterateAccessor<fastgltf::math::fvec2>(asset, accessor))
                    primitive.vertices[vertex_index++].texture_coordinate = {value[0], value[1]};
            }
            const fastgltf::Attribute* uv1 = source_primitive.findAttribute("TEXCOORD_1");
            if (uv1 != source_primitive.attributes.end()) {
                const fastgltf::Accessor& accessor = asset.accessors[uv1->accessorIndex];
                if (accessor.count != primitive.vertices.size() || accessor.type != fastgltf::AccessorType::Vec2)
                    return std::unexpected{"UV1 count mismatch"};
                vertex_index = 0;
                for (fastgltf::math::fvec2 value : fastgltf::iterateAccessor<fastgltf::math::fvec2>(asset, accessor))
                    primitive.vertices[vertex_index++].texture_coordinate_1 = {value[0], value[1]};
            }
            if (primitive.material != no_asset_index) {
                const ImportedMaterial& material = scene.materials[primitive.material];
                const std::array<uint32, render::material_texture_count> textures = material_textures(material);
                for (size_t slot = 0; slot < textures.size(); ++slot) {
                    if (textures[slot] == no_asset_index)
                        continue;
                    if ((material.surface.mapping[slot].uv_set == 1 && uv1 == source_primitive.attributes.end()) ||
                        (material.surface.mapping[slot].uv_set == 0 && texture_attribute == source_primitive.attributes.end()))
                        return std::unexpected{"Material references an absent UV channel"};
                }
            }
            if (!source_primitive.indicesAccessor) {
                return std::unexpected{"glTF index generation failed"};
            }
            const fastgltf::Accessor& index_accessor = asset.accessors[*source_primitive.indicesAccessor];
            if (!count_fits(index_accessor.count) || index_accessor.type != fastgltf::AccessorType::Scalar) {
                return std::unexpected{"glTF primitive has too many indices"};
            }
            primitive.indices.resize(index_accessor.count);
            fastgltf::copyFromAccessor<uint32>(asset, index_accessor, primitive.indices.data());
            if (std::expected<MeshProcessingMetrics, std::string> processed =
                    process_imported_primitive(primitive, {.source_normals = normal_attribute != source_primitive.attributes.end(),
                                                              .source_texture_coordinates = texture_attribute != source_primitive.attributes.end()});
                !processed) {
                return std::unexpected{"Could not process glTF mesh: " + processed.error()};
            }
            scene.primitives.push_back(std::move(primitive));
        }
        mesh.primitive_count = static_cast<uint32>(scene.primitives.size() - mesh.first_primitive);
        scene.meshes.push_back(std::move(mesh));
    }

    scene.nodes.reserve(asset.nodes.size());
    for (const fastgltf::Node& source_node : asset.nodes) {
        ImportedNode node{.name = std::string{source_node.name}, .mesh = optional_index(source_node.meshIndex), .skin = optional_index(source_node.skinIndex)};
        const fastgltf::math::fmat4x4 transform = fastgltf::getTransformMatrix(source_node);
        for (size_t column = 0; column < 4; ++column) {
            for (size_t row = 0; row < 4; ++row) {
                node.local_transform[column * 4 + row] = transform[column][row];
            }
        }
        node.children.reserve(source_node.children.size());
        for (const size_t child : source_node.children) {
            node.children.push_back(static_cast<uint32>(child));
        }
        scene.nodes.push_back(std::move(node));
    }
    scene.scenes.reserve(asset.scenes.size());
    for (const fastgltf::Scene& source_scene : asset.scenes) {
        ImportedSceneDefinition definition{.name = std::string{source_scene.name}};
        definition.roots.reserve(source_scene.nodeIndices.size());
        for (const size_t root : source_scene.nodeIndices) {
            definition.roots.push_back(static_cast<uint32>(root));
        }
        scene.scenes.push_back(std::move(definition));
    }
    scene.skins.reserve(asset.skins.size());
    for (const fastgltf::Skin& source_skin : asset.skins) {
        ImportedSkin skin{.name = std::string{source_skin.name}};
        skin.joints.reserve(source_skin.joints.size());
        for (size_t j : source_skin.joints)
            skin.joints.push_back(static_cast<uint32>(j));
        if (skin.joints.empty() || skin.joints.size() > 256 || !source_skin.inverseBindMatrices)
            return std::unexpected{"A bind-pose skin requires 1..256 joints and inverse bind matrices"};
        const fastgltf::Accessor& accessor = asset.accessors[*source_skin.inverseBindMatrices];
        if (accessor.count != skin.joints.size() || accessor.type != fastgltf::AccessorType::Mat4)
            return std::unexpected{"Invalid inverse bind matrix count or type"};
        skin.inverse_bind_matrices.resize(skin.joints.size());
        size_t matrix_index = 0;
        for (fastgltf::math::fmat4x4 value : fastgltf::iterateAccessor<fastgltf::math::fmat4x4>(asset, accessor)) {
            for (size_t c = 0; c < 4; ++c)
                for (size_t r = 0; r < 4; ++r)
                    skin.inverse_bind_matrices[matrix_index][c * 4 + r] = value[c][r];
            ++matrix_index;
        }
        scene.skins.push_back(std::move(skin));
    }
    size_t animation_keys = 0;
    scene.animations.reserve(asset.animations.size());
    for (const fastgltf::Animation& animation : asset.animations) {
        AnimationClip clip{.name = std::string{animation.name}};
        clip.channels.reserve(animation.channels.size());
        for (const fastgltf::AnimationChannel& source_channel : animation.channels) {
            if (!source_channel.nodeIndex || source_channel.samplerIndex >= animation.samplers.size() ||
                source_channel.path == fastgltf::AnimationPath::Weights)
                return std::unexpected{"Unsupported animation target"};
            const fastgltf::AnimationSampler& sampler = animation.samplers[source_channel.samplerIndex];
            if (sampler.interpolation != fastgltf::AnimationInterpolation::Linear && sampler.interpolation != fastgltf::AnimationInterpolation::Step)
                return std::unexpected{"Only LINEAR and STEP animation are supported; bake cubic curves offline"};
            if (!std::holds_alternative<fastgltf::TRS>(asset.nodes[*source_channel.nodeIndex].transform))
                return std::unexpected{"Animated nodes must use TRS"};
            const fastgltf::Accessor& input = asset.accessors[sampler.inputAccessor];
            const fastgltf::Accessor& output = asset.accessors[sampler.outputAccessor];
            const bool rotation = source_channel.path == fastgltf::AnimationPath::Rotation;
            animation_keys += input.count;
            if (animation_keys > maximum_elements || input.count == 0 || input.count != output.count || input.type != fastgltf::AccessorType::Scalar ||
                input.componentType != fastgltf::ComponentType::Float ||
                output.type != (rotation ? fastgltf::AccessorType::Vec4 : fastgltf::AccessorType::Vec3) ||
                output.componentType != fastgltf::ComponentType::Float)
                return std::unexpected{"Invalid animation accessor layout"};
            AnimationChannel channel{.node = static_cast<uint32>(*source_channel.nodeIndex),
                .path = rotation                                                      ? AnimationPath::rotation
                        : source_channel.path == fastgltf::AnimationPath::Translation ? AnimationPath::translation
                                                                                      : AnimationPath::scale,
                .interpolation =
                    sampler.interpolation == fastgltf::AnimationInterpolation::Step ? AnimationInterpolation::step : AnimationInterpolation::linear};
            channel.times.resize(input.count);
            fastgltf::copyFromAccessor<float>(asset, input, channel.times.data());
            channel.values.reserve(output.count);
            if (rotation)
                for (fastgltf::math::fvec4 value : fastgltf::iterateAccessor<fastgltf::math::fvec4>(asset, output))
                    channel.values.push_back({value[0], value[1], value[2], value[3]});
            else
                for (fastgltf::math::fvec3 value : fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, output))
                    channel.values.push_back({value[0], value[1], value[2], 0});
            if (clip.duration < channel.times.back())
                clip.duration = channel.times.back();
            clip.channels.push_back(std::move(channel));
        }
        scene.animations.push_back(std::move(clip));
    }
    if (!validate_imported_scene(scene)) {
        return std::unexpected{"Imported glTF contains invalid cross references"};
    }
    return scene;
}
#else
std::vector<std::byte> encode_imported_scene(const ImportedScene& scene) {
    assert(validate_imported_scene(scene));
    std::vector<std::byte> output;
    output.reserve(encoded_scene_size(scene));
    output.insert(output.end(), scene_magic.begin(), scene_magic.end());
    append_integer(output, scene_payload_version);
    append_integer(output, scene.default_scene);
    append_integer(output, static_cast<uint32>(scene.primitives.size()));
    append_integer(output, static_cast<uint32>(scene.meshes.size()));
    append_integer(output, static_cast<uint32>(scene.materials.size()));
    append_integer(output, static_cast<uint32>(scene.textures.size()));
    append_integer(output, static_cast<uint32>(scene.images.size()));
    append_integer(output, static_cast<uint32>(scene.nodes.size()));
    append_integer(output, static_cast<uint32>(scene.scenes.size()));
    append_integer(output, static_cast<uint32>(scene.skins.size()));
    append_integer(output, static_cast<uint32>(scene.animations.size()));
    for (const ImportedPrimitive& primitive : scene.primitives) {
        append_integer(output, primitive.material);
        append_integer(output, static_cast<uint32>(primitive.vertices.size()));
        append_integer(output, static_cast<uint32>(primitive.indices.size()));
        for (const ImportedVertex& vertex : primitive.vertices) {
            for (const float value : vertex.position)
                append_float(output, value);
            for (const float value : vertex.normal)
                append_float(output, value);
            for (const float value : vertex.texture_coordinate)
                append_float(output, value);
            for (const float value : vertex.tangent)
                append_float(output, value);
            for (const float value : vertex.texture_coordinate_1)
                append_float(output, value);
            for (uint16 value : vertex.joints)
                append_integer(output, value);
            for (float value : vertex.weights)
                append_float(output, value);
        }
        for (const uint32 index : primitive.indices)
            append_integer(output, index);
        for (const float value : primitive.bounds_center)
            append_float(output, value);
        append_float(output, primitive.bounds_radius);
        append_integer(output, static_cast<uint32>(primitive.lod_indices.size()));
        for (const std::vector<uint32>& lod : primitive.lod_indices) {
            append_integer(output, static_cast<uint32>(lod.size()));
            for (uint32 index : lod)
                append_integer(output, index);
        }
    }
    for (const ImportedMesh& mesh : scene.meshes) {
        append_string(output, mesh.name);
        append_integer(output, mesh.first_primitive);
        append_integer(output, mesh.primitive_count);
    }
    for (const ImportedMaterial& material : scene.materials) {
        append_string(output, material.name);
        for (const float value : material.base_color)
            append_float(output, value);
        for (const float value : material.emissive)
            append_float(output, value);
        append_float(output, material.metallic);
        append_float(output, material.roughness);
        append_float(output, material.alpha_cutoff);
        append_integer(output, material.alpha_mode);
        append_integer(output, static_cast<uint8>(material.double_sided));
        append_integer(output, uint16{0});
        append_integer(output, material.base_color_texture);
        append_integer(output, material.metallic_roughness_texture);
        append_integer(output, material.normal_texture);
        for (const float* field : surface_fields(material.surface))
            append_float(output, *field);
        append_integer(output, material.surface.alpha_mode);
        append_integer(output, static_cast<uint8>(material.surface.double_sided));
        for (const render::TextureMapping& map : material.surface.mapping) {
            for (float value : {map.scale[0], map.scale[1], map.offset[0], map.offset[1], map.rotation})
                append_float(output, value);
            append_integer(output, map.uv_set);
        }
        for (uint32 texture : material.extra_textures)
            append_integer(output, texture);
    }
    for (const ImportedTexture& texture : scene.textures)
        append_integer(output, texture.image);
    for (const ImportedImage& image : scene.images) {
        append_string(output, image.name);
        append_string(output, image.external_uri);
        append_integer(output, static_cast<uint8>(image.embedded));
    }
    for (const ImportedNode& node : scene.nodes) {
        append_string(output, node.name);
        for (const float value : node.local_transform)
            append_float(output, value);
        append_integer(output, node.mesh);
        append_integer(output, node.skin);
        append_integer(output, static_cast<uint32>(node.children.size()));
        for (const uint32 child : node.children)
            append_integer(output, child);
    }
    for (const ImportedSceneDefinition& definition : scene.scenes) {
        append_string(output, definition.name);
        append_integer(output, static_cast<uint32>(definition.roots.size()));
        for (const uint32 root : definition.roots)
            append_integer(output, root);
    }
    for (const ImportedSkin& skin : scene.skins) {
        append_string(output, skin.name);
        append_integer(output, static_cast<uint32>(skin.joints.size()));
        for (uint32 joint : skin.joints)
            append_integer(output, joint);
        for (const RigMatrix& matrix : skin.inverse_bind_matrices)
            for (float value : matrix)
                append_float(output, value);
    }
    for (const AnimationClip& clip : scene.animations) {
        append_string(output, clip.name);
        append_float(output, clip.duration);
        append_integer(output, static_cast<uint32>(clip.channels.size()));
        for (const AnimationChannel& channel : clip.channels) {
            append_integer(output, channel.node);
            append_integer(output, static_cast<uint8>(channel.path));
            append_integer(output, static_cast<uint8>(channel.interpolation));
            append_integer(output, static_cast<uint32>(channel.times.size()));
            for (float t : channel.times)
                append_float(output, t);
            for (const std::array<float, 4>& v : channel.values)
                for (float x : v)
                    append_float(output, x);
        }
    }
    assert(output.size() == encoded_scene_size(scene));
    return output;
}

std::expected<ImportedScene, std::string> decode_imported_scene(const Span<const std::byte> encoded) {
    if (encoded.size() < scene_magic.size() || memcmp(scene_magic.data(), encoded.data(), scene_magic.size()) != 0) {
        return std::unexpected{"Cooked scene payload header is invalid"};
    }
    Reader reader{encoded.subspan(scene_magic.size())};
    const std::optional<uint32> version = reader.integer<uint32>();
    const std::optional<uint32> default_scene = reader.integer<uint32>();
    std::array<uint32, 9> counts{};
    for (uint32& count : counts) {
        const std::optional<uint32> value = reader.integer<uint32>();
        if (!value || *value > maximum_elements) {
            return std::unexpected{"Cooked scene payload counts are invalid"};
        }
        count = *value;
    }
    if (!version || *version != scene_payload_version || !default_scene) {
        return std::unexpected{"Cooked scene payload version is unsupported"};
    }
    const uint64 minimum_size = counts[0] * 32ULL + counts[1] * 12ULL + counts[2] * (141ULL + render::material_texture_count * 24ULL) + counts[3] * 4ULL +
                                counts[4] * 9ULL + counts[5] * 80ULL + counts[6] * 8ULL + counts[7] * 8ULL + counts[8] * 12ULL;
    if (minimum_size > reader.remaining())
        return std::unexpected{"Cooked scene counts exceed its payload"};
    ImportedScene scene;
    scene.default_scene = *default_scene;
    scene.primitives.reserve(counts[0]);
    for (uint32 primitive_index = 0; primitive_index < counts[0]; ++primitive_index) {
        const std::optional<uint32> material = reader.integer<uint32>();
        const std::optional<uint32> vertex_count = reader.integer<uint32>();
        const std::optional<uint32> index_count = reader.integer<uint32>();
        if (!material || !vertex_count || !index_count || *vertex_count > maximum_elements || *index_count > maximum_elements ||
            static_cast<uint64>(*vertex_count) * (22U * sizeof(float) + 8U * sizeof(uint16)) + static_cast<uint64>(*index_count) * sizeof(uint32) >
                reader.remaining()) {
            return std::unexpected{"Cooked scene primitive header is invalid"};
        }
        ImportedPrimitive primitive{.material = *material};
        primitive.vertices.resize(*vertex_count);
        for (ImportedVertex& vertex : primitive.vertices) {
            for (float& value : vertex.position) {
                const std::optional<float> decoded = reader.floating();
                if (!decoded)
                    return std::unexpected{"Cooked vertex is truncated"};
                value = *decoded;
            }
            for (float& value : vertex.normal) {
                const std::optional<float> decoded = reader.floating();
                if (!decoded)
                    return std::unexpected{"Cooked normal is truncated"};
                value = *decoded;
            }
            for (float& value : vertex.texture_coordinate) {
                const std::optional<float> decoded = reader.floating();
                if (!decoded)
                    return std::unexpected{"Cooked UV is truncated"};
                value = *decoded;
            }
            for (float& value : vertex.tangent) {
                const std::optional<float> decoded = reader.floating();
                if (!decoded)
                    return std::unexpected{"Cooked tangent is truncated"};
                value = *decoded;
            }
            for (float& value : vertex.texture_coordinate_1) {
                const std::optional<float> decoded = reader.floating();
                if (!decoded)
                    return std::unexpected{"Cooked UV1 truncated"};
                value = *decoded;
            }
            for (uint16& value : vertex.joints) {
                const std::optional<uint16> decoded = reader.integer<uint16>();
                if (!decoded)
                    return std::unexpected{"Cooked joints truncated"};
                value = *decoded;
            }
            for (float& value : vertex.weights) {
                const std::optional<float> decoded = reader.floating();
                if (!decoded)
                    return std::unexpected{"Cooked weights truncated"};
                value = *decoded;
            }
        }
        primitive.indices.resize(*index_count);
        for (uint32& index : primitive.indices) {
            const std::optional<uint32> decoded = reader.integer<uint32>();
            if (!decoded)
                return std::unexpected{"Cooked indices are truncated"};
            index = *decoded;
        }
        for (float& value : primitive.bounds_center) {
            const std::optional<float> decoded = reader.floating();
            if (!decoded)
                return std::unexpected{"Cooked mesh bounds are truncated"};
            value = *decoded;
        }
        const std::optional<float> bounds_radius = reader.floating();
        const std::optional<uint32> lod_count = reader.integer<uint32>();
        if (!bounds_radius || !lod_count || *bounds_radius < 0.0F || *lod_count > 2U)
            return std::unexpected{"Cooked mesh bounds or LOD count is invalid"};
        primitive.bounds_radius = *bounds_radius;
        primitive.lod_indices.reserve(*lod_count);
        for (uint32 lod = 0; lod < *lod_count; ++lod) {
            const std::optional<uint32> count = reader.integer<uint32>();
            if (!count || *count == 0 || *count > maximum_elements || static_cast<uint64>(*count) * sizeof(uint32) > reader.remaining())
                return std::unexpected{"Cooked mesh LOD is invalid"};
            std::vector<uint32>& indices = primitive.lod_indices.emplace_back(*count);
            for (uint32& index : indices) {
                const std::optional<uint32> decoded = reader.integer<uint32>();
                if (!decoded)
                    return std::unexpected{"Cooked mesh LOD is truncated"};
                index = *decoded;
            }
        }
        scene.primitives.push_back(std::move(primitive));
    }
    scene.meshes.reserve(counts[1]);
    for (uint32 index = 0; index < counts[1]; ++index) {
        std::optional<std::string> name = reader.string();
        const std::optional<uint32> first = reader.integer<uint32>();
        const std::optional<uint32> count = reader.integer<uint32>();
        if (!name || !first || !count)
            return std::unexpected{"Cooked mesh is truncated"};
        scene.meshes.push_back({.name = std::move(*name), .first_primitive = *first, .primitive_count = *count});
    }
    scene.materials.reserve(counts[2]);
    for (uint32 index = 0; index < counts[2]; ++index) {
        ImportedMaterial material;
        std::optional<std::string> name = reader.string();
        if (!name)
            return std::unexpected{"Cooked material is truncated"};
        material.name = std::move(*name);
        for (float& value : material.base_color) {
            const std::optional<float> decoded = reader.floating();
            if (!decoded)
                return std::unexpected{"Cooked material is truncated"};
            value = *decoded;
        }
        for (float& value : material.emissive) {
            const std::optional<float> decoded = reader.floating();
            if (!decoded)
                return std::unexpected{"Cooked material is truncated"};
            value = *decoded;
        }
        const std::optional<float> metallic = reader.floating();
        const std::optional<float> roughness = reader.floating();
        const std::optional<float> cutoff = reader.floating();
        const std::optional<uint8> alpha = reader.integer<uint8>();
        const std::optional<uint8> sided = reader.integer<uint8>();
        const std::optional<uint16> reserved = reader.integer<uint16>();
        const std::optional<uint32> base = reader.integer<uint32>();
        const std::optional<uint32> mr = reader.integer<uint32>();
        const std::optional<uint32> normal = reader.integer<uint32>();
        if (!metallic || !roughness || !cutoff || !alpha || !sided || !reserved || *reserved != 0 || !base || !mr || !normal || *sided > 1)
            return std::unexpected{"Cooked material fields are invalid"};
        material.metallic = *metallic;
        material.roughness = *roughness;
        material.alpha_cutoff = *cutoff;
        material.alpha_mode = *alpha;
        material.double_sided = *sided != 0;
        material.base_color_texture = *base;
        material.metallic_roughness_texture = *mr;
        material.normal_texture = *normal;
        for (float* field : surface_fields(material.surface)) {
            const std::optional<float> decoded = reader.floating();
            if (!decoded)
                return std::unexpected{"Cooked material surface truncated"};
            *field = *decoded;
        }
        const std::optional<uint32> mode = reader.integer<uint32>();
        const std::optional<uint8> two_sided = reader.integer<uint8>();
        if (!mode || !two_sided || *two_sided > 1)
            return std::unexpected{"Invalid surface flags"};
        material.surface.alpha_mode = *mode;
        material.surface.double_sided = *two_sided != 0;
        for (render::TextureMapping& map : material.surface.mapping) {
            for (float* field : {&map.scale[0], &map.scale[1], &map.offset[0], &map.offset[1], &map.rotation}) {
                const std::optional<float> decoded = reader.floating();
                if (!decoded)
                    return std::unexpected{"Cooked texture mapping truncated"};
                *field = *decoded;
            }
            const std::optional<uint32> uv = reader.integer<uint32>();
            if (!uv)
                return std::unexpected{"Cooked UV selector truncated"};
            map.uv_set = *uv;
        }
        for (uint32& texture : material.extra_textures) {
            const std::optional<uint32> decoded = reader.integer<uint32>();
            if (!decoded)
                return std::unexpected{"Cooked extra texture truncated"};
            texture = *decoded;
        }
        scene.materials.push_back(std::move(material));
    }
    scene.textures.reserve(counts[3]);
    for (uint32 index = 0; index < counts[3]; ++index) {
        const std::optional<uint32> image = reader.integer<uint32>();
        if (!image)
            return std::unexpected{"Cooked texture is truncated"};
        scene.textures.push_back({.image = *image});
    }
    scene.images.reserve(counts[4]);
    for (uint32 index = 0; index < counts[4]; ++index) {
        std::optional<std::string> name = reader.string();
        std::optional<std::string> uri = reader.string();
        const std::optional<uint8> embedded = reader.integer<uint8>();
        if (!name || !uri || !embedded || *embedded > 1)
            return std::unexpected{"Cooked image is invalid"};
        scene.images.push_back({.name = std::move(*name), .external_uri = std::move(*uri), .embedded = *embedded != 0});
    }
    scene.nodes.reserve(counts[5]);
    for (uint32 index = 0; index < counts[5]; ++index) {
        ImportedNode node;
        std::optional<std::string> name = reader.string();
        if (!name)
            return std::unexpected{"Cooked node is truncated"};
        node.name = std::move(*name);
        for (float& value : node.local_transform) {
            const std::optional<float> decoded = reader.floating();
            if (!decoded)
                return std::unexpected{"Cooked node transform is truncated"};
            value = *decoded;
        }
        const std::optional<uint32> mesh = reader.integer<uint32>();
        const std::optional<uint32> skin = reader.integer<uint32>();
        const std::optional<uint32> child_count = reader.integer<uint32>();
        if (!mesh || !skin || !child_count || *child_count > maximum_elements || static_cast<uint64>(*child_count) * sizeof(uint32) > reader.remaining())
            return std::unexpected{"Cooked node fields are invalid"};
        node.mesh = *mesh;
        node.skin = *skin;
        node.children.resize(*child_count);
        for (uint32& child : node.children) {
            const std::optional<uint32> decoded = reader.integer<uint32>();
            if (!decoded)
                return std::unexpected{"Cooked node children are truncated"};
            child = *decoded;
        }
        scene.nodes.push_back(std::move(node));
    }
    scene.scenes.reserve(counts[6]);
    for (uint32 index = 0; index < counts[6]; ++index) {
        ImportedSceneDefinition definition;
        std::optional<std::string> name = reader.string();
        const std::optional<uint32> root_count = reader.integer<uint32>();
        if (!name || !root_count || *root_count > maximum_elements || static_cast<uint64>(*root_count) * sizeof(uint32) > reader.remaining())
            return std::unexpected{"Cooked scene definition is invalid"};
        definition.name = std::move(*name);
        definition.roots.resize(*root_count);
        for (uint32& root : definition.roots) {
            const std::optional<uint32> decoded = reader.integer<uint32>();
            if (!decoded)
                return std::unexpected{"Cooked scene roots are truncated"};
            root = *decoded;
        }
        scene.scenes.push_back(std::move(definition));
    }
    scene.skins.reserve(counts[7]);
    for (uint32 i = 0; i < counts[7]; ++i) {
        std::optional<std::string> name = reader.string();
        const std::optional<uint32> count = reader.integer<uint32>();
        if (!name || !count || *count == 0 || *count > 256 || reader.remaining() < *count * 68ULL)
            return std::unexpected{"Invalid cooked skin header"};
        ImportedSkin skin{.name = std::move(*name)};
        skin.joints.reserve(*count);
        for (uint32 j = 0; j < *count; ++j)
            skin.joints.push_back(*reader.integer<uint32>());
        skin.inverse_bind_matrices.resize(*count);
        for (RigMatrix& matrix : skin.inverse_bind_matrices)
            for (float& value : matrix) {
                const std::optional<float> decoded = reader.floating();
                if (!decoded)
                    return std::unexpected{"Invalid inverse bind matrix"};
                value = *decoded;
            }
        scene.skins.push_back(std::move(skin));
    }
    size_t animation_keys = 0;
    if (counts[8] > 256)
        return std::unexpected{"Too many cooked clips"};
    scene.animations.reserve(counts[8]);
    for (uint32 i = 0; i < counts[8]; ++i) {
        std::optional<std::string> name = reader.string();
        const std::optional<float> duration = reader.floating();
        const std::optional<uint32> count = reader.integer<uint32>();
        if (!name || !duration || !count || *count > scene.nodes.size() * 3 || reader.remaining() < *count * 10ULL)
            return std::unexpected{"Invalid cooked animation header"};
        AnimationClip clip{.name = std::move(*name), .duration = *duration};
        clip.channels.reserve(*count);
        for (uint32 c = 0; c < *count; ++c) {
            const std::optional<uint32> node = reader.integer<uint32>();
            const std::optional<uint8> path = reader.integer<uint8>();
            const std::optional<uint8> mode = reader.integer<uint8>();
            const std::optional<uint32> keys = reader.integer<uint32>();
            if (!node || !path || !mode || !keys || *keys > maximum_elements || reader.remaining() < *keys * 20ULL)
                return std::unexpected{"Invalid cooked animation channel"};
            animation_keys += *keys;
            if (animation_keys > maximum_elements)
                return std::unexpected{"Cooked animation key budget exceeded"};
            AnimationChannel channel{.node = *node, .path = static_cast<AnimationPath>(*path), .interpolation = static_cast<AnimationInterpolation>(*mode)};
            channel.times.reserve(*keys);
            for (uint32 k = 0; k < *keys; ++k) {
                const std::optional<float> t = reader.floating();
                if (!t)
                    return std::unexpected{"Invalid animation time"};
                channel.times.push_back(*t);
            }
            channel.values.resize(*keys);
            for (std::array<float, 4>& v : channel.values)
                for (float& x : v) {
                    const std::optional<float> f = reader.floating();
                    if (!f)
                        return std::unexpected{"Invalid animation value"};
                    x = *f;
                }
            clip.channels.push_back(std::move(channel));
        }
        scene.animations.push_back(std::move(clip));
    }
    if (reader.remaining() != 0 || !validate_imported_scene(scene)) {
        return std::unexpected{"Cooked scene payload has trailing or invalid data"};
    }
    return scene;
}
#endif

} // namespace gloom::assets

#if !defined(GLOOM_GLTF_IMPORTER)
namespace gloom::assets {
namespace {
bool animation_name_contains(Span<const char> name, const char* part) {
    for (size_t i = 0; i + strlen(part) <= name.size(); ++i)
        if (memcmp(name.data() + i, part, strlen(part)) == 0)
            return true;
    return false;
}
bool hierarchy(const ImportedScene& scene, Array<PreparedNode>& nodes, Array<uint32>& order) {
    if (scene.nodes.size() > ~0U)
        return false;
    nodes.reserve(scene.nodes.size());
    nodes.resize(scene.nodes.size());
    order.reserve(scene.nodes.size());
    order.resize(scene.nodes.size());
    for (uint32 i = 0; i < scene.nodes.size(); ++i)
        for (uint32 child : scene.nodes[i].children) {
            if (child >= nodes.size() || nodes[child].parent != no_animation_index)
                return false;
            nodes[child].parent = i;
        }
    size_t count = 0;
    for (uint32 i = 0; i < nodes.size(); ++i)
        if (nodes[i].parent == no_animation_index)
            order[count++] = i;
    for (size_t head = 0; head < count; ++head)
        for (uint32 child : scene.nodes[order[head]].children)
            order[count++] = child;
    return count == nodes.size();
}
bool valid_trs(const RigMatrix& m) {
    for (float v : m)
        if (!isfinite(v))
            return false;
    const float d = m[0] * (m[5] * m[10] - m[9] * m[6]) - m[4] * (m[1] * m[10] - m[9] * m[2]) + m[8] * (m[1] * m[6] - m[5] * m[2]);
    return isfinite(d) && fabsf(d) >= 1e-12F && hypot(hypot(m[0], m[1]), m[2]) >= 1e-8 && hypot(hypot(m[4], m[5]), m[6]) >= 1e-8 &&
           hypot(hypot(m[8], m[9]), m[10]) >= 1e-8;
}
bool clips_valid(const ImportedScene& scene) {
    if (scene.animations.size() > 256)
        return false;
    size_t keys = 0;
    for (size_t c = 0; c < scene.animations.size(); ++c) {
        const AnimationClip& clip = scene.animations[c];
        if (clip.name.empty() || !isfinite(clip.duration) || clip.duration <= 0 || clip.duration > 3600 || clip.channels.empty() ||
            clip.channels.size() > scene.nodes.size() * 3)
            return false;
        for (size_t j = 0; j < c; ++j)
            if (scene.animations[j].name.size() == clip.name.size() && memcmp(scene.animations[j].name.data(), clip.name.data(), clip.name.size()) == 0)
                return false;
        for (size_t j = 0; j < clip.channels.size(); ++j) {
            const AnimationChannel& channel = clip.channels[j];
            if (channel.node >= scene.nodes.size() || channel.path > AnimationPath::scale || channel.interpolation > AnimationInterpolation::step ||
                channel.times.empty() || channel.times.size() != channel.values.size())
                return false;
            for (size_t k = 0; k < j; ++k)
                if (clip.channels[k].node == channel.node && clip.channels[k].path == channel.path)
                    return false;
            keys += channel.times.size();
            if (keys > 1'000'000 || !valid_trs(scene.nodes[channel.node].local_transform))
                return false;
            const RigMatrix rebuilt = rig_matrix(rig_transform(scene.nodes[channel.node].local_transform));
            for (size_t k = 0; k < 16; ++k)
                if (fabsf(scene.nodes[channel.node].local_transform[k] - rebuilt[k]) > 1e-3F)
                    return false;
            float previous = -1;
            for (size_t k = 0; k < channel.times.size(); ++k) {
                const float t = channel.times[k];
                if (!isfinite(t) || t < 0 || t <= previous || t > clip.duration)
                    return false;
                previous = t;
                for (float v : channel.values[k])
                    if (!isfinite(v))
                        return false;
                if (channel.path == AnimationPath::rotation &&
                    fabsf(channel.values[k][0] * channel.values[k][0] + channel.values[k][1] * channel.values[k][1] +
                          channel.values[k][2] * channel.values[k][2] + channel.values[k][3] * channel.values[k][3] - 1) > 1e-3F)
                    return false;
                if (channel.path == AnimationPath::scale && (channel.values[k][0] < 1e-4F || channel.values[k][1] < 1e-4F || channel.values[k][2] < 1e-4F))
                    return false;
            }
        }
    }
    return true;
}
}
bool valid_animations(const ImportedScene& scene) {
    if (scene.animations.empty())
        return true;
    Array<PreparedNode> nodes;
    Array<uint32> order;
    if (!hierarchy(scene, nodes, order) || !clips_valid(scene))
        return false;
    Array<RigMatrix> worlds;
    worlds.reserve(nodes.size());
    worlds.resize(nodes.size());
    for (uint32 i : order) {
        worlds[i] =
            nodes[i].parent == no_animation_index ? scene.nodes[i].local_transform : rig_multiply(worlds[nodes[i].parent], scene.nodes[i].local_transform);
        for (float value : worlds[i])
            if (!isfinite(value))
                return false;
    }
    return true;
}
bool prepare_animation_rig(const ImportedScene& scene, AnimationRig& rig, uint64 generation) {
    // ImportedScene owns its STL containers under the 117 exception; prepare once, never copy per frame.
    AnimationRig prepared;
    if (scene.nodes.empty() || !hierarchy(scene, prepared.nodes, prepared.order) || !clips_valid(scene) || !valid_bind_rigs(scene))
        return false;
    size_t bytes = 0;
    for (const ImportedNode& node : scene.nodes) {
        if (!valid_trs(node.local_transform))
            return false;
        bytes += node.name.size() + 1;
    }
    for (const AnimationClip& clip : scene.animations)
        bytes += clip.name.size() + 1;
    if (bytes > ~0U)
        return false;
    prepared.names.reserve(bytes);
    prepared.names.resize(bytes);
    prepared.rest.reserve(scene.nodes.size());
    prepared.rest.resize(scene.nodes.size());
    size_t cursor = 0;
    for (size_t i = 0; i < scene.nodes.size(); ++i) {
        prepared.nodes[i].name = static_cast<uint32>(cursor);
        prepared.nodes[i].name_length = static_cast<uint32>(scene.nodes[i].name.size());
        prepared.nodes[i].skin = scene.nodes[i].skin;
        memcpy(prepared.names.data() + cursor, scene.nodes[i].name.c_str(), scene.nodes[i].name.size() + 1);
        // Importer-owned strings retain their full length, including embedded zero bytes.
        prepared.nodes[i].finger_rotation = animation_name_contains({scene.nodes[i].name.data(), scene.nodes[i].name.size()}, "Finger")
                                                ? (animation_name_contains({scene.nodes[i].name.data(), scene.nodes[i].name.size()}, " R ") ? .6F : -.6F)
                                                : 0;
        cursor += scene.nodes[i].name.size() + 1;
        prepared.rest[i] = rig_transform(scene.nodes[i].local_transform);
    }
    prepared.bind_worlds.reserve(prepared.nodes.size());
    prepared.bind_worlds.resize(prepared.nodes.size());
    pose_worlds(prepared, {prepared.rest.data(), prepared.rest.size()}, {prepared.bind_worlds.data(), prepared.bind_worlds.size()});
    for (const RigMatrix& world : prepared.bind_worlds)
        if (!valid_trs(world))
            return false;
    prepared.clips.reserve(scene.animations.size());
    prepared.clips.resize(scene.animations.size());
    for (size_t i = 0; i < scene.animations.size(); ++i) {
        PreparedClip& clip = prepared.clips[i];
        const AnimationClip& source = scene.animations[i];
        clip.name = static_cast<uint32>(cursor);
        clip.name_length = static_cast<uint32>(source.name.size());
        memcpy(prepared.names.data() + cursor, source.name.c_str(), source.name.size() + 1);
        cursor += source.name.size() + 1;
        clip.duration = source.duration;
        clip.channels.reserve(source.channels.size());
        clip.channels.resize(source.channels.size());
        for (size_t j = 0; j < source.channels.size(); ++j) {
            PreparedChannel& channel = clip.channels[j];
            channel.node = source.channels[j].node;
            channel.path = static_cast<uint8>(source.channels[j].path);
            channel.interpolation = static_cast<uint8>(source.channels[j].interpolation);
            channel.keys.reserve(source.channels[j].times.size());
            channel.keys.resize(source.channels[j].times.size());
            for (size_t k = 0; k < channel.keys.size(); ++k)
                channel.keys[k] = {.time = source.channels[j].times[k],
                    .value = {
                        source.channels[j].values[k][0], source.channels[j].values[k][1], source.channels[j].values[k][2], source.channels[j].values[k][3]}};
        }
    }
    prepared.skins.reserve(scene.skins.size());
    prepared.skins.resize(scene.skins.size());
    for (size_t i = 0; i < scene.skins.size(); ++i) {
        PreparedSkin& skin = prepared.skins[i];
        skin.joints.reserve(scene.skins[i].joints.size());
        skin.joints.resize(scene.skins[i].joints.size());
        skin.inverse_bind.reserve(skin.joints.size());
        skin.inverse_bind.resize(skin.joints.size());
        for (size_t j = 0; j < skin.joints.size(); ++j) {
            skin.joints[j] = scene.skins[i].joints[j];
            skin.inverse_bind[j] = scene.skins[i].inverse_bind_matrices[j];
            if (!valid_trs(skin.inverse_bind[j]))
                return false;
        }
    }
    prepared.bounds.reserve(scene.primitives.size());
    prepared.bounds.resize(scene.primitives.size());
    for (size_t i = 0; i < scene.primitives.size(); ++i) {
        for (const ImportedVertex& vertex : scene.primitives[i].vertices) {
            for (float value : vertex.position)
                if (!isfinite(value))
                    return false;
            for (uint32 j = 0; j < 8; ++j)
                if (!isfinite(vertex.weights[j]) || vertex.weights[j] < 0 || (vertex.weights[j] > 0 && vertex.joints[j] >= 256))
                    return false;
        }
        prepare_skin_bounds(scene.primitives[i], prepared.bounds[i]);
    }
    prepared.generation = generation;
    static int64 identity = 0;
#ifdef _WIN32
    prepared.identity = static_cast<uint64>(InterlockedIncrement64(&identity));
#else
    prepared.identity = static_cast<uint64>(__atomic_add_fetch(&identity, 1, __ATOMIC_RELAXED));
#endif
    rig = static_cast<AnimationRig&&>(prepared);
    return true;
}
render::Vec3 skinned_position(const ImportedVertex& v, const render::SkinPose& pose) {
    render::Vec3 p{};
    for (size_t i = 0; i < 8; ++i)
        if (v.weights[i] > 0) {
            const RigMatrix& m = pose.matrices[v.joints[i]];
            const float w = v.weights[i];
            p.x += w * (m[0] * v.position[0] + m[4] * v.position[1] + m[8] * v.position[2] + m[12]);
            p.y += w * (m[1] * v.position[0] + m[5] * v.position[1] + m[9] * v.position[2] + m[13]);
            p.z += w * (m[2] * v.position[0] + m[6] * v.position[1] + m[10] * v.position[2] + m[14]);
        }
    return p;
}

void prepare_skin_bounds(const ImportedPrimitive& primitive, SkinBounds& bounds) {
    bounds = {};
    for (const ImportedVertex& vertex : primitive.vertices) {
        float weight_sum = 0;
        for (uint32 i = 0; i < 8; ++i) {
            assert(vertex.weights[i] >= 0);
            if (vertex.weights[i] == 0)
                continue;
            assert(vertex.joints[i] < 256);
            SkinBounds::Joint& joint = bounds.joints[vertex.joints[i]];
            joint.minimum = {
                fminf(joint.minimum.x, vertex.position[0]), fminf(joint.minimum.y, vertex.position[1]), fminf(joint.minimum.z, vertex.position[2])};
            joint.maximum = {
                fmaxf(joint.maximum.x, vertex.position[0]), fmaxf(joint.maximum.y, vertex.position[1]), fmaxf(joint.maximum.z, vertex.position[2])};
            if (bounds.joint_count <= vertex.joints[i])
                bounds.joint_count = vertex.joints[i] + 1;
            weight_sum += vertex.weights[i];
        }
        bounds.minimum_weight_sum = fminf(bounds.minimum_weight_sum, weight_sum);
        bounds.maximum_weight_sum = fmaxf(bounds.maximum_weight_sum, weight_sum);
    }
}

render::BoundingSphere skinned_bounds(const ImportedPrimitive& primitive, const render::SkinPose& pose) {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::bounds);
    render::Vec3 lo{1e30F, 1e30F, 1e30F}, hi{-1e30F, -1e30F, -1e30F};
    for (const ImportedVertex& v : primitive.vertices) {
        const render::Vec3 p = skinned_position(v, pose);
        lo = {fminf(lo.x, p.x), fminf(lo.y, p.y), fminf(lo.z, p.z)};
        hi = {fmaxf(hi.x, p.x), fmaxf(hi.y, p.y), fmaxf(hi.z, p.z)};
    }
    return {.center = {(lo.x + hi.x) * .5F, (lo.y + hi.y) * .5F, (lo.z + hi.z) * .5F},
        .radius = static_cast<float>(hypot(hypot(hi.x - lo.x, hi.y - lo.y), hi.z - lo.z)) * .5F + 1e-3F};
}
} // namespace gloom::assets
#endif
