#pragma once

#include <gloom/assets/asset.hpp>
#include <gloom/assets/gltf_importer.hpp>
#include <gloom/render/gpu_assets.hpp>
#include <gloom/render/scene.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace gloom::assets {

struct GpuScenePrimitive {
    render::RenderAssetId mesh;
    render::RenderAssetId material;
    render::BoundingSphere bounds;
    std::array<render::RenderAssetId, 3> lod_meshes{};
    std::uint8_t lod_count{1};
    render::RenderAssetId arms_mesh;
};

struct GpuSceneUploads {
    std::vector<render::MeshUpload> meshes;
    std::vector<render::MaterialUpload> materials;
    std::vector<GpuScenePrimitive> primitives;
};

// Converts the backend-neutral cooked scene representation into owned upload
// packets. It performs no GPU work and is therefore safe on loader/job threads.
[[nodiscard]] GpuSceneUploads build_gpu_scene_uploads(const ImportedScene& scene,
                                                      AssetId scene_asset,
                                                      std::span<const render::RenderAssetId>
                                                          image_assets = {});

} // namespace gloom::assets
