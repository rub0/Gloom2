#pragma once

#include <gloom/render/gpu_assets.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <memory>
#include <vector>

namespace gloom::render {

struct ClusteredLightingView;

struct Vec3 {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};

struct Quaternion {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
    float w{1.0F};
};

struct Transform {
    Vec3 position;
    Quaternion rotation;
    Vec3 scale{1.0F, 1.0F, 1.0F};
};

struct Color {
    float red{1.0F};
    float green{1.0F};
    float blue{1.0F};
    float alpha{1.0F};
};

struct Camera {
    Vec3 position{7.0F, 5.0F, -9.0F};
    Vec3 target{0.0F, 1.5F, 0.0F};
    Vec3 up{0.0F, 1.0F, 0.0F};
    float vertical_field_of_view_radians{0.785398163F};
    float near_plane{0.1F};
    float far_plane{500.0F};
};

struct BoundingSphere {
    Vec3 center;
    float radius{1.7320508F};
};

struct SkinPose {
    // Column-major asset matrices, equivalent to row-major transposed HLSL matrices.
    std::vector<std::array<float, 16>> matrices;
    std::vector<std::array<float, 16>> normal_matrices;
};

struct RenderInstance {
    RenderAssetId mesh{builtin_cube_mesh};
    RenderAssetId material{builtin_default_material};
    Transform transform;
    // Supplying the previous presentation transform yields object motion vectors.
    // Static and newly spawned instances can leave this false.
    Transform previous_transform;
    bool has_previous_transform{false};
    Color color;
    BoundingSphere local_bounds;
    std::array<RenderAssetId, 3> lod_meshes{};
    std::uint8_t lod_count{1};
    // Camera-attached props draw after the world with their own depth and never
    // cast or receive world shadows. Their placement still follows the camera.
    bool view_model{false};
    bool casts_shadow{true};
    std::uint32_t source_node{~0U};
    std::uint32_t source_primitive{~0U};
    std::shared_ptr<const SkinPose> pose;
    std::shared_ptr<const SkinPose> previous_pose;
    RenderAssetId arms_mesh;
    bool particle{false};
    float distortion{0},soft_distance{0};
};

struct RenderBatch {
    RenderAssetId mesh;
    RenderAssetId material;
    std::uint32_t first_instance{0};
    std::uint32_t instance_count{0};
};

struct UiDrawData;
struct RenderSnapshot {
    float presentation_seconds{0.0F};
    Camera camera;
    std::span<const RenderInstance> instances;
    std::span<const RenderBatch> batches;
    const ClusteredLightingView* lighting{nullptr};
    std::span<const RenderInstance> shadow_instances;
    const UiDrawData* ui{nullptr};
};

[[nodiscard]] Transform interpolate(const Transform& previous,
                                    const Transform& current,
                                    float alpha) noexcept;
[[nodiscard]] Transform camera_relative_transform(const Camera& camera, Vec3 offset, Vec3 scale);
// For TRS, inverse-transpose of the linear part is reciprocal scale then rotation.
[[nodiscard]] Transform normal_transform(const Transform& transform) noexcept;
// Exact TRS composition for the uniform-scale attachment parents used by art.
[[nodiscard]] Transform attach_transform(const Transform& parent, const Transform& local) noexcept;

} // namespace gloom::render
