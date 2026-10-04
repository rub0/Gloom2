#pragma once

#include <gloom/render/scene.hpp>

namespace gloom::assets {
struct ImportedScene;
struct ImportedPrimitive;
struct ImportedVertex;
using RigMatrix = Matrix4;
using LocalPose = Array<render::Transform>;
inline constexpr uint32 no_animation_index = ~0U;
// Matrix helpers require finite affine matrices; inverse needs |det| >= 1e-12,
// and decomposition needs column lengths >= 1e-8. Validated once at import/binding.
[[nodiscard]] RigMatrix rig_multiply(const RigMatrix& a, const RigMatrix& b);
[[nodiscard]] RigMatrix rig_matrix(const render::Transform& transform);
[[nodiscard]] RigMatrix rig_inverse(const RigMatrix& matrix);
[[nodiscard]] render::Transform rig_transform(const RigMatrix& matrix);
[[nodiscard]] bool valid_animations(const ImportedScene& scene);
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
struct PreparedNode {
    uint32 parent{no_animation_index}, skin{no_animation_index}, name{0}, name_length{0};
    float finger_rotation{0};
};
struct AnimationKey {
    float time{0};
    render::Quaternion value;
};
struct PreparedChannel {
    uint32 node{no_animation_index};
    uint8 path{0}, interpolation{0};
    Array<AnimationKey> keys;
};
struct PreparedClip {
    uint32 name{0}, name_length{0};
    float duration{0};
    Array<PreparedChannel> channels;
};
struct PreparedSkin {
    Array<uint32> joints;
    Array<RigMatrix> inverse_bind;
};
// Immutable after preparation. Replacing/destroying a rig invalidates its animator views;
// do so only after their last CPU consumer. Runtime sampling never reads importer containers.
struct AnimationRig {
    uint64 generation{0};
    uint64 identity{0}; // Unique prepared lifetime, even when an allocator reuses an address.
    Array<char> names;
    Array<PreparedNode> nodes;
    Array<uint32> order;
    LocalPose rest;
    Array<RigMatrix> bind_worlds;
    Array<PreparedClip> clips;
    Array<PreparedSkin> skins;
    Array<SkinBounds> bounds;
};
// External-input boundary: rejects malformed hierarchy, animation, bind matrices and weights.
[[nodiscard]] bool prepare_animation_rig(const ImportedScene& scene, AnimationRig& rig, uint64 generation = 1);
[[nodiscard]] uint32 animation_node(const AnimationRig& rig, const char* name);
// Outputs have the prepared rig's node count. Caller-owned scratch permits concurrent actors.
// Blend permits exact input/output aliasing. Span arguments are never retained.
void rest_pose(const AnimationRig& rig, Span<render::Transform> pose);
void sample_animation(const AnimationRig& rig, const char* name, double seconds, Span<render::Transform> pose, bool loop = true);
void blend_poses(Span<const render::Transform> a, Span<const render::Transform> b, float weight, Span<render::Transform> result);
void pose_worlds(const AnimationRig& rig, Span<const render::Transform> pose, Span<RigMatrix> result);
// Reserve only at binding/growth, outside measured evaluation. skin_pose never grows its output.
void prepare_skin_pose(const AnimationRig& rig, uint32 mesh_node, render::SkinPose& pose);
void skin_pose(const AnimationRig& rig, uint32 mesh_node, Span<const RigMatrix> worlds, render::SkinPose& pose);
// Preconditions: finite vertices, nonnegative weights, joints < 256 and covered by the pose.
void prepare_skin_bounds(const ImportedPrimitive& primitive, SkinBounds& bounds);
[[nodiscard]] render::BoundingSphere skinned_bounds(const SkinBounds& bounds, const render::SkinPose& pose);
} // namespace gloom::assets
