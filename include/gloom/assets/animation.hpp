#pragma once

#include <gloom/assets/rig.hpp>
#include <gloom/render/scene.hpp>
#include <string_view>

namespace gloom::assets {
using LocalPose = std::vector<render::Transform>;
[[nodiscard]] RigMatrix rig_matrix(const render::Transform& transform);
[[nodiscard]] RigMatrix rig_inverse(const RigMatrix& matrix);
[[nodiscard]] render::Transform rig_transform(const RigMatrix& matrix);
[[nodiscard]] bool valid_animations(const ImportedScene& scene);
[[nodiscard]] LocalPose rest_pose(const ImportedScene& scene);
// Sampling clamps non-looping clips, wraps looping clips and uses quaternion slerp.
[[nodiscard]] LocalPose sample_animation(const ImportedScene& scene, std::string_view name,
                                        double seconds, bool loop = true);
[[nodiscard]] LocalPose blend_poses(const LocalPose& a, const LocalPose& b, float weight);
[[nodiscard]] std::vector<RigMatrix> pose_worlds(const ImportedScene& scene, const LocalPose& pose);
[[nodiscard]] std::shared_ptr<render::SkinPose> skin_pose(const ImportedScene& scene,
    std::uint32_t mesh_node, const std::vector<RigMatrix>& worlds);
[[nodiscard]] render::BoundingSphere skinned_bounds(const ImportedPrimitive& primitive,
                                                    const render::SkinPose& pose);
[[nodiscard]] render::Vec3 skinned_position(const ImportedVertex& vertex, const render::SkinPose& pose);
} // namespace gloom::assets
