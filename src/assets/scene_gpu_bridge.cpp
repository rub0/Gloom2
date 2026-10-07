#include <gloom/assets/scene_gpu_bridge.hpp>
#include <string.h>

namespace gloom::assets {
namespace {
bool contains(Span<const char> name, const char* token, size_t length) {
    if (length > name.size())
        return false;
    for (size_t index = 0; index <= name.size() - length; ++index)
        if (memcmp(name.data() + index, token, length) == 0)
            return true;
    return false;
}
render::RenderAssetId child_id(AssetId parent, uint32 kind, uint32 index) noexcept {
    uint64 value = parent.value ^ (static_cast<uint64>(kind) << 32U) ^ index;
    value ^= value >> 30U;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27U;
    value *= 0x94d049bb133111ebULL;
    value ^= value >> 31U;
    return {.value = value == 0 ? 0x9e3779b97f4a7c15ULL : value};
}
render::RenderAssetId texture_asset(const ImportedScene& scene, Span<const render::RenderAssetId> images, uint32 texture, render::RenderAssetId fallback) {
    if (texture == no_asset_index || texture >= scene.textures.size())
        return fallback;
    const uint32 image = scene.textures[texture].image;
    return image < images.size() && images[image].value ? images[image] : fallback;
}
}
GpuSceneUploads build_gpu_scene_uploads(const ImportedScene& scene, AssetId scene_asset, Span<const render::RenderAssetId> image_assets) {
    GpuSceneUploads result;
    result.materials.reserve(scene.materials.size());
    for (uint32 index = 0; index < scene.materials.size(); ++index) {
        const ImportedMaterial& source = scene.materials[index];
        render::MaterialUpload material{.id = child_id(scene_asset, 2, index),
            .base_color = source.base_color,
            .emissive = source.emissive,
            .metallic = source.metallic,
            .roughness = source.roughness,
            .base_color_texture = texture_asset(scene, image_assets, source.base_color_texture, render::builtin_white_texture),
            .metallic_roughness_texture = texture_asset(scene, image_assets, source.metallic_roughness_texture, render::builtin_white_texture),
            .normal_texture = texture_asset(scene, image_assets, source.normal_texture, render::builtin_flat_normal_texture),
            .surface = source.surface};
        for (size_t slot = 0; slot < source.extra_textures.size(); ++slot)
            material.extra_textures[slot] = texture_asset(scene, image_assets, source.extra_textures[slot], render::builtin_white_texture);
        result.materials.push_back(static_cast<render::MaterialUpload&&>(material));
    }
    Array<uint8> arm;
    if (scene.skins.size() == 1) {
        arm.reserve(scene.skins[0].joints.size());
        arm.resize(scene.skins[0].joints.size());
        for (size_t joint = 0; joint < arm.size(); ++joint) {
            assert(scene.skins[0].joints[joint] < scene.nodes.size());
            const Span<const char> name{scene.nodes[scene.skins[0].joints[joint]].name};
            arm[joint] = contains(name, "UpperArm", 8) || contains(name, "Forearm", 7) || contains(name, "Hand", 4) || contains(name, "Finger", 6);
        }
    }
    size_t count = scene.primitives.size() + (scene.skins.size() == 1 ? scene.primitives.size() : 0);
    for (const ImportedPrimitive& primitive : scene.primitives)
        count += primitive.lod_indices.size();
    result.meshes.reserve(count);
    result.primitives.reserve(scene.primitives.size());
    for (uint32 index = 0; index < scene.primitives.size(); ++index) {
        const ImportedPrimitive& source = scene.primitives[index];
        assert(source.lod_indices.size() < 3);
        render::MeshUpload mesh{.id = child_id(scene_asset, 1, index)};
        mesh.vertices.reserve(source.vertices.size());
        for (const ImportedVertex& vertex : source.vertices)
            mesh.vertices.push_back({.position = vertex.position,
                .normal = vertex.normal,
                .texture_coordinate = vertex.texture_coordinate,
                .tangent = vertex.tangent,
                .texture_coordinate_1 = vertex.texture_coordinate_1,
                .joints = vertex.joints,
                .weights = vertex.weights});
        mesh.indices = source.indices; // Renderer upload packets still belong to 119; each packet owns its data.
        GpuScenePrimitive binding{.mesh = mesh.id,
            .material = source.material == no_asset_index ? render::builtin_default_material : child_id(scene_asset, 2, source.material),
            .bounds = {.center = {.x = source.bounds_center[0], .y = source.bounds_center[1], .z = source.bounds_center[2]}, .radius = source.bounds_radius},
            .lod_count = static_cast<uint8>(1U + source.lod_indices.size())};
        binding.lod_meshes[0] = mesh.id;
        if (scene.skins.size() == 1) {
            render::MeshUpload arms{.id = child_id(scene_asset, 20, index)};
            arms.indices.reserve(source.indices.size());
            for (size_t triangle = 0; triangle + 2 < source.indices.size(); triangle += 3) {
                bool keep = true;
                for (size_t corner = 0; corner < 3; ++corner) {
                    assert(source.indices[triangle + corner] < source.vertices.size());
                    const ImportedVertex& vertex = source.vertices[source.indices[triangle + corner]];
                    float weight = 0;
                    for (size_t joint = 0; joint < 8; ++joint)
                        if (vertex.joints[joint] < arm.size() && arm[vertex.joints[joint]])
                            weight += vertex.weights[joint];
                    keep = keep && weight > .95F;
                }
                if (keep)
                    for (size_t corner = 0; corner < 3; ++corner)
                        arms.indices.push_back(source.indices[triangle + corner]);
            }
            if (!arms.indices.empty()) {
                arms.vertices = mesh.vertices;
                binding.arms_mesh = arms.id;
                result.meshes.push_back(static_cast<render::MeshUpload&&>(arms));
            }
        }
        const size_t base = result.meshes.size();
        result.meshes.push_back(static_cast<render::MeshUpload&&>(mesh));
        for (size_t lod = 0; lod < source.lod_indices.size(); ++lod) {
            render::MeshUpload lod_mesh{.id = child_id(scene_asset, static_cast<uint32>(10U + lod), index),
                .vertices = result.meshes[base].vertices,
                .indices = source.lod_indices[lod]};
            binding.lod_meshes[lod + 1U] = lod_mesh.id;
            result.meshes.push_back(static_cast<render::MeshUpload&&>(lod_mesh));
        }
        result.primitives.push_back(binding);
    }
    return result;
}
}
