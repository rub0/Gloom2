#include <gloom/core/allocation_profile.hpp>
#include <gloom/assets/animation.hpp>
#include <assert.h>
#include <math.h>
#include <string.h>

namespace gloom::assets {
namespace {
render::Quaternion slerp(render::Quaternion a, render::Quaternion b, float t) {
    float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (d < 0) {
        b = {-b.x, -b.y, -b.z, -b.w};
        d = -d;
    }
    if (d > .9995F)
        return render::interpolate({.rotation = a}, {.rotation = b}, t).rotation;
    const float angle = acosf(fminf(fmaxf(d, -1.0F), 1.0F));
    const float u = sinf((1 - t) * angle) / sinf(angle), v = sinf(t * angle) / sinf(angle);
    return {a.x * u + b.x * v, a.y * u + b.y * v, a.z * u + b.z * v, a.w * u + b.w * v};
}
}

RigMatrix rig_matrix(const render::Transform& t) {
    const float x = t.rotation.x, y = t.rotation.y, z = t.rotation.z, w = t.rotation.w;
    return {(1 - 2 * (y * y + z * z)) * t.scale.x, 2 * (x * y + z * w) * t.scale.x, 2 * (x * z - y * w) * t.scale.x, 0, 2 * (x * y - z * w) * t.scale.y,
        (1 - 2 * (x * x + z * z)) * t.scale.y, 2 * (y * z + x * w) * t.scale.y, 0, 2 * (x * z + y * w) * t.scale.z, 2 * (y * z - x * w) * t.scale.z,
        (1 - 2 * (x * x + y * y)) * t.scale.z, 0, t.position.x, t.position.y, t.position.z, 1};
}

RigMatrix rig_inverse(const RigMatrix& m) {
    const float d = m[0] * (m[5] * m[10] - m[9] * m[6]) - m[4] * (m[1] * m[10] - m[9] * m[2]) + m[8] * (m[1] * m[6] - m[5] * m[2]);
    assert(isfinite(d) && fabsf(d) >= 1e-12F);
    RigMatrix r{(m[5] * m[10] - m[9] * m[6]) / d, (m[9] * m[2] - m[1] * m[10]) / d, (m[1] * m[6] - m[5] * m[2]) / d, 0, (m[8] * m[6] - m[4] * m[10]) / d,
        (m[0] * m[10] - m[8] * m[2]) / d, (m[4] * m[2] - m[0] * m[6]) / d, 0, (m[4] * m[9] - m[8] * m[5]) / d, (m[8] * m[1] - m[0] * m[9]) / d,
        (m[0] * m[5] - m[4] * m[1]) / d, 0, 0, 0, 0, 1};
    for (size_t i = 0; i < 3; ++i)
        r[12 + i] = -r[i] * m[12] - r[4 + i] * m[13] - r[8 + i] * m[14];
    return r;
}

render::Transform rig_transform(const RigMatrix& m) {
    render::Vec3 scale{static_cast<float>(hypot(hypot(m[0], m[1]), m[2])), static_cast<float>(hypot(hypot(m[4], m[5]), m[6])),
        static_cast<float>(hypot(hypot(m[8], m[9]), m[10]))};
    assert(scale.x >= 1e-8F && scale.y >= 1e-8F && scale.z >= 1e-8F);
    if (m[0] * (m[5] * m[10] - m[9] * m[6]) - m[4] * (m[1] * m[10] - m[9] * m[2]) + m[8] * (m[1] * m[6] - m[5] * m[2]) < 0)
        scale.x = -scale.x;
    const render::Camera c{.position = {}, .target = {m[8] / scale.z, m[9] / scale.z, m[10] / scale.z}, .up = {m[4] / scale.y, m[5] / scale.y, m[6] / scale.y}};
    const render::Quaternion rotation = render::camera_relative_transform(c, {}, {1, 1, 1}).rotation;
    return {.position = {m[12], m[13], m[14]}, .rotation = rotation, .scale = scale};
}

RigMatrix rig_multiply(const RigMatrix& a, const RigMatrix& b) {
    RigMatrix result{};
    for (size_t c = 0; c < 4; ++c)
        for (size_t r = 0; r < 4; ++r)
            for (size_t k = 0; k < 4; ++k)
                result[c * 4 + r] += a[k * 4 + r] * b[c * 4 + k];
    return result;
}

uint32 animation_node(const AnimationRig& rig, const char* name) {
    for (uint32 i = 0; i < rig.nodes.size(); ++i)
        if (strlen(name) == rig.nodes[i].name_length && memcmp(rig.names.data() + rig.nodes[i].name, name, rig.nodes[i].name_length) == 0)
            return i;
    return no_animation_index;
}
void rest_pose(const AnimationRig& rig, Span<render::Transform> pose) {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::poses);
    assert(pose.size() == rig.rest.size());
    for (size_t i = 0; i < pose.size(); ++i)
        pose[i] = rig.rest[i];
}
void sample_animation(const AnimationRig& rig, const char* name, double seconds, Span<render::Transform> pose, bool loop) {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::poses);
    assert(isfinite(seconds));
    rest_pose(rig, pose);
    for (const PreparedClip& clip : rig.clips) {
        if (strlen(name) != clip.name_length || memcmp(rig.names.data() + clip.name, name, clip.name_length) != 0)
            continue;
        const float t = static_cast<float>(loop ? fmod(fmax(0.0, seconds), clip.duration) : fmin(fmax(seconds, 0.0), clip.duration));
        for (const PreparedChannel& c : clip.channels) {
            // Upper bound, including the original before-first-key behavior.
            size_t lo = 0, hi = c.keys.size();
            while (lo < hi) {
                const size_t mid = lo + (hi - lo) / 2;
                if (t < c.keys[mid].time)
                    hi = mid;
                else
                    lo = mid + 1;
            }
            const size_t a = lo ? lo - 1 : 0, b = a + 1 < c.keys.size() ? a + 1 : a;
            const float w = a == b || c.interpolation == 1 ? 0 : fminf(fmaxf((t - c.keys[a].time) / (c.keys[b].time - c.keys[a].time), 0), 1);
            const render::Quaternion& x = c.keys[a].value;
            const render::Quaternion& y = c.keys[b].value;
            const render::Vec3 v{x.x + (y.x - x.x) * w, x.y + (y.y - x.y) * w, x.z + (y.z - x.z) * w};
            if (c.path == 0)
                pose[c.node].position = v;
            else if (c.path == 2)
                pose[c.node].scale = v;
            else
                pose[c.node].rotation = slerp(x, y, w);
        }
        return;
    }
}
void blend_poses(Span<const render::Transform> a, Span<const render::Transform> b, float weight, Span<render::Transform> result) {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::poses);
    assert(a.size() == b.size() && result.size() == a.size() && isfinite(weight));
    const float t = fminf(fmaxf(weight, 0), 1);
    for (size_t i = 0; i < a.size(); ++i) {
        // Cache rotations before writing when result aliases either input.
        const render::Quaternion rotation = slerp(a[i].rotation, b[i].rotation, t);
        result[i] = render::interpolate(a[i], b[i], t);
        result[i].rotation = rotation;
    }
}
void pose_worlds(const AnimationRig& rig, Span<const render::Transform> pose, Span<RigMatrix> result) {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::poses);
    assert(pose.size() == rig.nodes.size() && result.size() == pose.size());
    for (uint32 i : rig.order) {
        const RigMatrix local = rig_matrix(pose[i]);
        result[i] = rig.nodes[i].parent == no_animation_index ? local : rig_multiply(result[rig.nodes[i].parent], local);
    }
}
void prepare_skin_pose(const AnimationRig& rig, uint32 mesh_node, render::SkinPose& pose) {
    assert(mesh_node < rig.nodes.size() && rig.nodes[mesh_node].skin < rig.skins.size());
    const size_t count = rig.skins[rig.nodes[mesh_node].skin].joints.size();
    pose.matrices.reserve(count);
    pose.matrices.resize(count);
    pose.normal_matrices.reserve(count);
    pose.normal_matrices.resize(count);
}
void skin_pose(const AnimationRig& rig, uint32 mesh_node, Span<const RigMatrix> worlds, render::SkinPose& pose) {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::poses);
    assert(worlds.size() == rig.nodes.size() && mesh_node < rig.nodes.size() && rig.nodes[mesh_node].skin < rig.skins.size());
    const RigMatrix inverse = rig_inverse(worlds[mesh_node]);
    const PreparedSkin& skin = rig.skins[rig.nodes[mesh_node].skin];
    assert(pose.matrices.size() == skin.joints.size() && pose.normal_matrices.size() == skin.joints.size());
    for (size_t i = 0; i < skin.joints.size(); ++i) {
        pose.matrices[i] = rig_multiply(inverse, rig_multiply(worlds[skin.joints[i]], skin.inverse_bind[i]));
        const RigMatrix inv = rig_inverse(pose.matrices[i]);
        for (size_t c = 0; c < 4; ++c)
            for (size_t r = 0; r < 4; ++r)
                pose.normal_matrices[i][c * 4 + r] = inv[r * 4 + c];
    }
}
render::BoundingSphere skinned_bounds(const SkinBounds& bounds, const render::SkinPose& pose) {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::bounds);
    assert(bounds.joint_count <= pose.matrices.size());
    if (!bounds.joint_count)
        return {.radius = .001F};
    render::Vec3 lo{1e30F, 1e30F, 1e30F}, hi{-1e30F, -1e30F, -1e30F};
    for (uint32 i = 0; i < bounds.joint_count; ++i) {
        const SkinBounds::Joint& joint = bounds.joints[i];
        if (joint.minimum.x > joint.maximum.x)
            continue;
        const render::Vec3 center{
            (joint.minimum.x + joint.maximum.x) * .5F, (joint.minimum.y + joint.maximum.y) * .5F, (joint.minimum.z + joint.maximum.z) * .5F};
        const render::Vec3 extent{
            (joint.maximum.x - joint.minimum.x) * .5F, (joint.maximum.y - joint.minimum.y) * .5F, (joint.maximum.z - joint.minimum.z) * .5F};
        const RigMatrix& m = pose.matrices[i];
        const render::Vec3 transformed{m[0] * center.x + m[4] * center.y + m[8] * center.z + m[12], m[1] * center.x + m[5] * center.y + m[9] * center.z + m[13],
            m[2] * center.x + m[6] * center.y + m[10] * center.z + m[14]};
        const render::Vec3 radius{fabsf(m[0]) * extent.x + fabsf(m[4]) * extent.y + fabsf(m[8]) * extent.z,
            fabsf(m[1]) * extent.x + fabsf(m[5]) * extent.y + fabsf(m[9]) * extent.z,
            fabsf(m[2]) * extent.x + fabsf(m[6]) * extent.y + fabsf(m[10]) * extent.z};
        lo = {fminf(lo.x, transformed.x - radius.x), fminf(lo.y, transformed.y - radius.y), fminf(lo.z, transformed.z - radius.z)};
        hi = {fmaxf(hi.x, transformed.x + radius.x), fmaxf(hi.y, transformed.y + radius.y), fmaxf(hi.z, transformed.z + radius.z)};
    }
    // Account for imported sums that differ from one, including unweighted vertices at the origin.
    lo = {fminf(lo.x * bounds.minimum_weight_sum, lo.x * bounds.maximum_weight_sum), fminf(lo.y * bounds.minimum_weight_sum, lo.y * bounds.maximum_weight_sum),
        fminf(lo.z * bounds.minimum_weight_sum, lo.z * bounds.maximum_weight_sum)};
    hi = {fmaxf(hi.x * bounds.minimum_weight_sum, hi.x * bounds.maximum_weight_sum), fmaxf(hi.y * bounds.minimum_weight_sum, hi.y * bounds.maximum_weight_sum),
        fmaxf(hi.z * bounds.minimum_weight_sum, hi.z * bounds.maximum_weight_sum)};
    return {.center = {(lo.x + hi.x) * .5F, (lo.y + hi.y) * .5F, (lo.z + hi.z) * .5F},
        .radius = sqrtf((hi.x - lo.x) * (hi.x - lo.x) + (hi.y - lo.y) * (hi.y - lo.y) + (hi.z - lo.z) * (hi.z - lo.z)) * .5F + .001F};
}

}
