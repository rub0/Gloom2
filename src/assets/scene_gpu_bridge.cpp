#include <gloom/assets/scene_gpu_bridge.hpp>

#include <bit>

namespace gloom::assets {
namespace {

[[nodiscard]] render::RenderAssetId child_id(const AssetId parent,
                                             const std::uint32_t kind,
                                             const std::uint32_t index) noexcept {
    std::uint64_t value = parent.value ^ (static_cast<std::uint64_t>(kind) << 32U) ^ index;
    value ^= value >> 30U;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27U;
    value *= 0x94d049bb133111ebULL;
    value ^= value >> 31U;
    return {value == 0 ? 0x9e3779b97f4a7c15ULL : value};
}

} // namespace

GpuSceneUploads build_gpu_scene_uploads(
    const ImportedScene& scene,
    const AssetId scene_asset,
    const std::span<const render::RenderAssetId> image_assets) {
    GpuSceneUploads result;
    const auto texture_asset = [&](const std::uint32_t texture_index,
                                   const render::RenderAssetId fallback) {
        if (texture_index == no_asset_index || texture_index >= scene.textures.size()) {
            return fallback;
        }
        const auto image = scene.textures[texture_index].image;
        return image < image_assets.size() && image_assets[image].value != 0
                   ? image_assets[image]
                   : fallback;
    };
    result.materials.reserve(scene.materials.size());
    for (std::uint32_t index = 0; index < scene.materials.size(); ++index) {
        const auto& source = scene.materials[index];
        std::array<render::RenderAssetId,7> extras;
        for (std::size_t slot=0;slot<extras.size();++slot)
            extras[slot]=texture_asset(source.extra_textures[slot],render::builtin_white_texture);
        result.materials.push_back({
            .id = child_id(scene_asset, 2, index),
            .base_color = source.base_color,
            .emissive = source.emissive,
            .metallic = source.metallic,
            .roughness = source.roughness,
            .base_color_texture =
                texture_asset(source.base_color_texture, render::builtin_white_texture),
            .metallic_roughness_texture = texture_asset(
                source.metallic_roughness_texture, render::builtin_white_texture),
            .normal_texture =
                texture_asset(source.normal_texture, render::builtin_flat_normal_texture),
            .surface = source.surface,
            .extra_textures = extras,
        });
    }

    std::size_t mesh_upload_count = scene.primitives.size();
    for (const auto& primitive : scene.primitives) {
        mesh_upload_count += primitive.lod_indices.size();
    }
    result.meshes.reserve(mesh_upload_count);
    result.primitives.reserve(scene.primitives.size());
    for (std::uint32_t index = 0; index < scene.primitives.size(); ++index) {
        const auto& source = scene.primitives[index];
        render::MeshUpload mesh{.id = child_id(scene_asset, 1, index)};
        mesh.vertices.reserve(source.vertices.size());
        for (const auto& vertex : source.vertices) {
            mesh.vertices.push_back({
                .position = vertex.position,
                .normal = vertex.normal,
                .texture_coordinate = vertex.texture_coordinate,
                .tangent = vertex.tangent,
                .texture_coordinate_1 = vertex.texture_coordinate_1,
                .joints = vertex.joints,
                .weights = vertex.weights,
            });
        }
        mesh.indices = source.indices;
        const auto material = source.material == no_asset_index
                                  ? render::builtin_default_material
                                  : child_id(scene_asset, 2, source.material);
        GpuScenePrimitive binding{
            .mesh = mesh.id,
            .material = material,
            .bounds = {.center = {source.bounds_center[0],
                                  source.bounds_center[1],
                                  source.bounds_center[2]},
                       .radius = source.bounds_radius},
            .lod_count = static_cast<std::uint8_t>(1U + source.lod_indices.size()),
        };
        binding.lod_meshes[0] = mesh.id;
        // Reuse recovered geometry and materials for FPS arms. Keep only whole
        // triangles whose vertices are controlled by arm/hand bones; no mesh
        // clipping shader, detached torso triangles or new replacement art.
        if (scene.skins.size()==1) {
            std::vector<bool> arm(scene.skins[0].joints.size());
            for (std::size_t j=0;j<arm.size();++j) {
                const auto& name=scene.nodes[scene.skins[0].joints[j]].name;
                arm[j]=name.find("UpperArm")!=std::string::npos || name.find("Forearm")!=std::string::npos ||
                    name.find("Hand")!=std::string::npos || name.find("Finger")!=std::string::npos;
            }
            auto arms=mesh;arms.id=child_id(scene_asset,20,index);arms.indices.clear();
            for (std::size_t t=0;t+2<source.indices.size();t+=3) {
                bool keep=true;
                for (std::size_t c=0;c<3;++c) {
                    const auto& v=source.vertices[source.indices[t+c]];float weight=0;
                    for (std::size_t j=0;j<8;++j) if (v.joints[j]<arm.size() && arm[v.joints[j]]) weight+=v.weights[j];
                    keep=keep && weight>.95F;
                }
                if (keep) arms.indices.insert(arms.indices.end(),source.indices.begin()+t,source.indices.begin()+t+3);
            }
            if (!arms.indices.empty()) {binding.arms_mesh=arms.id;result.meshes.push_back(std::move(arms));}
        }
        result.meshes.push_back(std::move(mesh));
        for (std::size_t lod = 0; lod < source.lod_indices.size(); ++lod) {
            auto lod_mesh = result.meshes.back();
            lod_mesh.id = child_id(scene_asset, static_cast<std::uint32_t>(10U + lod), index);
            lod_mesh.indices = source.lod_indices[lod];
            binding.lod_meshes[lod + 1U] = lod_mesh.id;
            result.meshes.push_back(std::move(lod_mesh));
        }
        result.primitives.push_back(binding);
    }
    return result;
}

} // namespace gloom::assets
