#pragma once

#include <gloom/assets/rig.hpp>
#include <gloom/render/scene.hpp>
#include <gloom/core/types.hpp>
#include <string_view>

namespace gloom::assets {
using LocalPose = std::vector<render::Transform>;
[[nodiscard]] RigMatrix rig_matrix(const render::Transform& transform);
[[nodiscard]] RigMatrix rig_inverse(const RigMatrix& matrix);
[[nodiscard]] render::Transform rig_transform(const RigMatrix& matrix);
[[nodiscard]] bool valid_animations(const ImportedScene& scene);
[[nodiscard]] LocalPose rest_pose(const ImportedScene& scene);
// Sampling clamps non-looping clips, wraps looping clips and uses quaternion slerp.
[[nodiscard]] LocalPose sample_animation(const ImportedScene& scene, std::string_view name, double seconds, bool loop = true);
[[nodiscard]] LocalPose blend_poses(const LocalPose& a, const LocalPose& b, float weight);
[[nodiscard]] std::vector<RigMatrix> pose_worlds(const ImportedScene& scene, const LocalPose& pose);
[[nodiscard]] std::shared_ptr<render::SkinPose> skin_pose(const ImportedScene& scene, std::uint32_t mesh_node, const std::vector<RigMatrix>& worlds);
[[nodiscard]] render::BoundingSphere skinned_bounds(const ImportedPrimitive& primitive, const render::SkinPose& pose);
[[nodiscard]] render::Vec3 skinned_position(const ImportedVertex& vertex, const render::SkinPose& pose);
// Precompute from immutable geometry, then transform joint boxes instead of every vertex.
// The convex hull of transformed boxes covers linear skinning with nonnegative weights.
struct SkinBounds {
    struct Joint {
        render::Vec3 minimum{1e30F, 1e30F, 1e30F};
        render::Vec3 maximum{-1e30F, -1e30F, -1e30F};
    } joints[256];
    uint32 joint_count{0};
    float minimum_weight_sum{1.0F};
    float maximum_weight_sum{1.0F};
};
// Preconditions: finite vertices, nonnegative weights, joints < 256 and covered by the pose.
void prepare_skin_bounds(const ImportedPrimitive& primitive, SkinBounds& bounds);
[[nodiscard]] render::BoundingSphere skinned_bounds(const SkinBounds& bounds, const render::SkinPose& pose);
} // namespace gloom::assets
