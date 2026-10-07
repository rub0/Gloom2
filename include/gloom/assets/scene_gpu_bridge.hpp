#pragma once

#include <gloom/assets/asset.hpp>
#include <gloom/assets/gltf_importer.hpp>
#include <gloom/render/gpu_assets.hpp>
#include <gloom/render/scene.hpp>

namespace gloom::assets {

struct GpuScenePrimitive {
    render::RenderAssetId mesh;
    render::RenderAssetId material;
    render::BoundingSphere bounds;
    render::RenderAssetId lod_meshes[3]{};
    uint8 lod_count{1};
    render::RenderAssetId arms_mesh;
};

struct GpuSceneUploads {
    Array<render::MeshUpload> meshes;
    Array<render::MaterialUpload> materials;
    Array<GpuScenePrimitive> primitives;
};

// Converts the backend-neutral cooked scene representation into owned upload
// packets. It performs no GPU work and is therefore safe on loader/job threads.
// Requires at most two extra LODs and valid node/vertex indices used by arm classification,
// as guaranteed by import/decode. image_assets is consumed only during this call.
[[nodiscard]] GpuSceneUploads build_gpu_scene_uploads(const ImportedScene& scene, AssetId scene_asset, Span<const render::RenderAssetId> image_assets = {});

} // namespace gloom::assets
