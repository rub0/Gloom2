#include <gloom/gameplay/character_animation.hpp>
#include <gloom/core/allocation_profile.hpp>
#include <math.h>
#include <assert.h>

namespace gloom::gameplay {
namespace {
using render::Vec3;
Vec3 add(Vec3 a, Vec3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 sub(Vec3 a, Vec3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3 mul(Vec3 a, float s) {
    return {a.x * s, a.y * s, a.z * s};
}
float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
float length(Vec3 a) {
    return sqrtf(dot(a, a));
}
Vec3 unit(Vec3 a) {
    return mul(a, 1 / fmaxf(length(a), 1e-6F));
}
Vec3 point(const assets::RigMatrix& m) {
    return {m[12], m[13], m[14]};
}
Vec3 aim(Vec3 v, float pitch) {
    const float y = v.y - 1.35F, z = v.z;
    return {v.x, 1.35F + y * cosf(pitch) + z * sinf(pitch), z * cosf(pitch) - y * sinf(pitch)};
}
Vec3 anchor(const assets::AnimationRig& rig, Span<const assets::RigMatrix> worlds, const char* name, Vec3 fallback) {
    const uint32 i = assets::animation_node(rig, name);
    return i < worlds.size() ? point(worlds[i]) : fallback;
}
render::Quaternion product(render::Quaternion a, render::Quaternion b) {
    return render::attach_transform({.rotation = a}, {.rotation = b}).rotation;
}
render::Quaternion inverse(render::Quaternion q) {
    return {-q.x, -q.y, -q.z, q.w};
}
render::Quaternion between(Vec3 a, Vec3 b) {
    a = unit(a);
    b = unit(b);
    float w = 1 + dot(a, b);
    Vec3 v{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    if (w < 1e-5F) {
        v = unit(fabsf(a.x) < .9F ? Vec3{0, a.z, -a.y} : Vec3{-a.z, 0, a.x});
        w = 0;
    }
    const float n = sqrtf(dot(v, v) + w * w);
    return {v.x / n, v.y / n, v.z / n, w / n};
}
void rotate_local(Span<render::Transform> pose, uint32 i, Vec3 axis, float angle) {
    if (i >= pose.size())
        return;
    const float s = sinf(angle * .5F);
    pose[i].rotation = product(pose[i].rotation, {axis.x * s, axis.y * s, axis.z * s, cosf(angle * .5F)});
}
void aim_bone(const assets::AnimationRig& rig, Span<render::Transform> pose, uint32 joint, uint32 child, Vec3 target, Span<assets::RigMatrix> worlds) {
    assets::pose_worlds(rig, pose, worlds);
    const render::Quaternion delta = between(sub(point(worlds[child]), point(worlds[joint])), sub(target, point(worlds[joint])));
    const uint32 parent = rig.nodes[joint].parent;
    const render::Quaternion p = parent == assets::no_animation_index ? render::Quaternion{} : assets::rig_transform(worlds[parent]).rotation;
    pose[joint].rotation = product(product(product(inverse(p), delta), p), pose[joint].rotation);
}
void solve_arm(const assets::AnimationRig& rig, Span<render::Transform> pose, bool right, Vec3 target, Span<assets::RigMatrix> worlds) {
    const uint32 upper = assets::animation_node(rig, right ? "Bip001 R UpperArm" : "Bip001 L UpperArm");
    const uint32 fore = assets::animation_node(rig, right ? "Bip001 R Forearm" : "Bip001 L Forearm");
    const uint32 hand = assets::animation_node(rig, right ? "Bip001 R Hand" : "Bip001 L Hand");
    if (upper >= pose.size() || fore >= pose.size() || hand >= pose.size())
        return;
    assets::pose_worlds(rig, pose, worlds);
    const Vec3 shoulder = point(worlds[upper]);
    const float a = length(sub(point(worlds[fore]), shoulder)), b = length(sub(point(worlds[hand]), point(worlds[fore])));
    const Vec3 direction = unit(sub(target, shoulder));
    const float distance = fminf(fmaxf(length(sub(target, shoulder)), fabsf(a - b) + .001F), a + b - .001F);
    const float along = (a * a - b * b + distance * distance) / (2 * distance);
    const Vec3 pole = Vec3{right ? 1.0F : -1.0F, -.8F, -.3F};
    const Vec3 bend = unit(sub(pole, mul(direction, dot(pole, direction))));
    const Vec3 elbow = add(shoulder, add(mul(direction, along), mul(bend, sqrtf(fmaxf(0.0F, a * a - along * along)))));
    aim_bone(rig, pose, upper, fore, elbow, worlds);
    aim_bone(rig, pose, fore, hand, target, worlds);
}
void fps_shoulder(const assets::AnimationRig& rig, Span<render::Transform> pose, bool right, Span<assets::RigMatrix> worlds) {
    const uint32 joint = assets::animation_node(rig, right ? "Bip001 R UpperArm" : "Bip001 L UpperArm");
    if (joint >= pose.size())
        return;
    assets::pose_worlds(rig, pose, worlds);
    const uint32 parent = rig.nodes[joint].parent;
    if (parent != assets::no_animation_index) {
        // Keep the authored FPS shoulder target and limb lengths.
        const render::Transform target{.position = {right ? .36F : 0.F, right ? .94F : 1.F, right ? .10F : .20F}};
        pose[joint].position = point(assets::rig_multiply(assets::rig_inverse(worlds[parent]), assets::rig_matrix(target)));
    }
}
}

void CharacterAnimator::reset() noexcept {
    initialized_ = false;
    sampled_ = false;
    frame_ = {};
}

void CharacterAnimator::bind(const assets::AnimationRig& rig) {
    assert(rig.nodes.size() && rig.nodes.size() == rig.rest.size() && rig.nodes.size() == rig.order.size());
    if (rig_ == &rig && generation_ == rig.generation && identity_ == rig.identity)
        return;
    const size_t count = rig.nodes.size();
    local_.reserve(count);
    local_.resize(count);
    last_pose_.reserve(count);
    last_pose_.resize(count);
    death_pose_.reserve(count);
    death_pose_.resize(count);
    worlds_.reserve(count);
    worlds_.resize(count);
    for (assets::LocalPose& pose : scratch_) {
        pose.reserve(count);
        pose.resize(count);
    }
    for (Array<render::SkinPose>& slot : skins_) {
        slot.reserve(count);
        slot.resize(count);
        for (uint32 i = 0; i < count; ++i)
            if (rig.nodes[i].skin != assets::no_animation_index)
                assets::prepare_skin_pose(rig, i, slot[i]);
            else
                slot[i] = {};
    }
    rig_ = &rig;
    generation_ = rig.generation;
    identity_ = rig.identity;
    reset();
}

void CharacterAnimator::finish_pose(const assets::AnimationRig& rig) {
    assets::pose_worlds(rig, {local_.data(), local_.size()}, {worlds_.data(), worlds_.size()});
    skin_slot_ ^= 1;
    for (uint32 i = 0; i < rig.nodes.size(); ++i)
        if (rig.nodes[i].skin != assets::no_animation_index)
            assets::skin_pose(rig, i, {worlds_.data(), worlds_.size()}, skins_[skin_slot_][i]);
    frame_.local = {local_.data(), local_.size()};
    frame_.worlds = {worlds_.data(), worlds_.size()};
    frame_.skins = {skins_[skin_slot_].data(), skins_[skin_slot_].size()};
    frame_.generation = generation_;
    frame_.identity = identity_;
}

const CharacterAnimationFrame& CharacterAnimator::sample(const assets::AnimationRig& rig, uint64 actor, const char* clip, double seconds) {
    bind(rig);
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::poses);
    frame_.cut = !initialized_ || !sampled_ || frame_.actor != actor;
    frame_.actor = actor;
    assets::sample_animation(rig, clip, seconds, {local_.data(), local_.size()});
    finish_pose(rig);
    initialized_ = true;
    sampled_ = true;
    return frame_;
}

const CharacterAnimationFrame& CharacterAnimator::update(
    const assets::AnimationRig& rig, const CombatantView& view, double seconds, bool discontinuity, bool rest_only, bool first_person) {
    assert(isfinite(seconds) && seconds >= 0);
    bind(rig);
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::poses);
    Span<render::Transform> local{local_.data(), local_.size()};
    Span<assets::RigMatrix> worlds{worlds_.data(), worlds_.size()};
    CharacterAnimationFrame& frame = frame_;
    const float dt = static_cast<float>(fmin(seconds, .25));
    const bool cut = discontinuity || !initialized_ || sampled_ || previous_.entity != view.entity || previous_.character != view.character ||
                     previous_.alive != view.alive || previous_.deaths != view.deaths ||
                     hypot(hypot(view.position_x - previous_.position_x, view.position_y - previous_.position_y), view.position_z - previous_.position_z) > 3;
    if (cut) {
        if (!view.alive)
            for (size_t i = 0; i < local.size(); ++i)
                death_pose_[i] = initialized_ && !sampled_ && previous_.alive && previous_.entity == view.entity && previous_.character == view.character
                                     ? last_pose_[i]
                                     : rig.rest[i];
        time_ = 0;
        air_time_ = 0;
        death_time_ = 0;
        equip_time_ = 0;
        movement_ = 0;
        landing_ = 0;
        recoil_ = 0;
        damage_ = 0;
    }
    if (initialized_ && !cut) {
        if (!previous_.grounded && view.grounded)
            landing_ = 1;
        if (view.life < previous_.life || view.shield < previous_.shield)
            damage_ = 1;
        if (view.shot_sequence != previous_.shot_sequence)
            recoil_ = 1;
    }
    time_ += dt;
    equip_time_ += dt;
    air_time_ = view.grounded ? 0 : air_time_ + dt;
    death_time_ = view.alive ? 0 : death_time_ + dt;
    const float speed = hypotf(view.velocity_x, view.velocity_z);
    movement_ += (fminf(fmaxf(speed / 3.0F, 0), 1) - movement_) * (1 - expf(-12 * dt));
    landing_ = fmaxf(0.0F, landing_ - dt * 5);
    recoil_ = fmaxf(0.0F, recoil_ - dt * 7);
    damage_ = fmaxf(0.0F, damage_ - dt * 5);
    frame.cut = cut;
    Span<const render::Transform> rest{rig.rest.data(), rig.rest.size()};
    assets::rest_pose(rig, local);
    if (!rest_only) {
        const float forward = view.velocity_x * view.facing_x + view.velocity_z * view.facing_z;
        const float strafe = view.velocity_x * view.facing_z - view.velocity_z * view.facing_x;
        const double cycle = time_ * (.8 + fminf(speed / 5.0F, 1.0F));
        Span<render::Transform> idle{scratch_[0].data(), scratch_[0].size()};
        Span<render::Transform> walk{scratch_[1].data(), scratch_[1].size()};
        Span<render::Transform> sideways{scratch_[2].data(), scratch_[2].size()};
        assets::sample_animation(rig, "idle", time_, idle);
        assets::sample_animation(rig, "forward", forward < 0 ? 1000 - cycle : cycle, walk);
        assets::sample_animation(rig, "strafe_right", strafe < 0 ? 1000 - cycle : cycle, sideways);
        assets::blend_poses(walk, sideways, fabsf(strafe) / (fabsf(forward) + fabsf(strafe) + .001F), walk);
        assets::blend_poses(idle, walk, movement_, local);
        if (!view.grounded && rig.clips.size()) {
            assets::sample_animation(rig, "jump", fmin(air_time_, .3), idle, false);
            assets::blend_poses(local, idle, .75F, local);
        }
        // Visual root displacement never drives authoritative locomotion. Keep
        // the recovered pelvis origin (especially Shadow's explicit rebase).
        const uint32 root = assets::animation_node(rig, "Bip001");
        if (root < rest.size())
            local[root].position = rest[root].position;
        const uint32 spine = assets::animation_node(rig, "Bip001 Spine"), neck = assets::animation_node(rig, "Bip001 Neck");
        const float breath = .018F * sinf(static_cast<float>(time_) * 2.7F);
        rotate_local(local, spine, {0, 0, 1}, breath - damage_ * .10F - .25F * view.aim_pitch);
        rotate_local(local, neck, {0, 0, 1}, -.2F * view.aim_pitch);
        if (view.character == SliceCharacter::shadow) {
            rotate_local(local, assets::animation_node(rig, "Bip001 Tail"), {0, 1, 0}, (.09F + movement_ * .18F) * sinf(static_cast<float>(time_) * 4));
            rotate_local(local, assets::animation_node(rig, "Bip001 Tail1"), {0, 0, 1}, (.10F + movement_ * .20F) * sinf(static_cast<float>(time_) * 4 - 1.2F));
            rotate_local(local, spine, {0, 1, 0}, movement_ * .07F * sinf(static_cast<float>(time_) * 6));
        }
        local[0].position.y += .008F * sinf(static_cast<float>(time_) * 3) - landing_ * .055F;
        const float pitch = first_person ? 0 : view.aim_pitch;
        const float recoil = recoil_ * .055F;
        const float equip = static_cast<float>(fmax(0.0, .25 - equip_time_)) * .6F;
        Vec3 right{.23F, 1.18F - equip, .30F - recoil}, left{.20F, 1.18F - equip, .49F - recoil};
        right = aim(right, pitch);
        left = aim(left, pitch);
        if (view.alive) {
            if (first_person) {
                fps_shoulder(rig, local, true, worlds);
                fps_shoulder(rig, local, false, worlds);
            }
            solve_arm(rig, local, true, right, worlds);
            solve_arm(rig, local, false, left, worlds);
        }
        for (uint32 i = 0; i < rig.nodes.size(); ++i)
            if (rig.nodes[i].finger_rotation != 0)
                rotate_local(local, i, {0, 0, 1}, rig.nodes[i].finger_rotation);
        if (!view.alive) {
            // Retain the last live pose through the fall and hold it afterwards;
            // a corpse must not keep looping idle, breathing or arm IK.
            for (size_t i = 0; i < local.size(); ++i)
                local[i] = death_pose_[i];
            const float f = fminf(fmaxf(static_cast<float>(death_time_) / .65F, 0), 1);
            // Collapse in normalized presentation space, never rescale a live rig.
            const float angle = 1.35F * f;
            const render::Transform collapse{
                .position = {0, .75F * (1 - cosf(angle)) - .55F * f, -.75F * sinf(angle)}, .rotation = {sinf(angle * .5F), 0, 0, cosf(angle * .5F)}};
            local[0] = assets::rig_transform(assets::rig_multiply(assets::rig_matrix(collapse), assets::rig_matrix(local[0])));
        }
    }
    finish_pose(rig);
    frame.actor = view.entity;
    const Vec3 hand = anchor(rig, frame.worlds, "Bip001 R Hand", {.23F, 1.18F, .35F});
    frame.left_grip = anchor(rig, frame.worlds, "Bip001 L Hand", {-.1F, 1.18F, .58F});
    const float pitch = first_person ? 0 : view.aim_pitch;
    frame.weapon = {.position = hand, .rotation = {-sinf(pitch * .5F), 0, 0, cosf(pitch * .5F)}, .scale = {.43F, .43F, .43F}};
    // Original Soul Reaper grip is an authored mesh-space socket.
    frame.weapon = render::attach_transform(frame.weapon, {.position = {-.13F, -.25F, -.20F}});
    // Recovered barrel tip is x=.21, y=.35, z=.93 in the normalized mesh.
    // Start beyond that surface so depth testing cannot bury the flash in the gun.
    frame.muzzle = render::attach_transform(frame.weapon, {.position = {.21F, .35F, 1.02F}}).position;
    frame.head = anchor(rig, frame.worlds, "Bip001 Head", {0, 1.7F, 0});
    frame.tail = anchor(rig, frame.worlds, "Bip001 Tail1", {0, .3F, 0});
    frame.left_wing = anchor(rig, frame.worlds, "Bip001 L Clavicle", {-.3F, 1.5F, 0});
    frame.right_wing = anchor(rig, frame.worlds, "Bip001 R Clavicle", {.3F, 1.5F, 0});
    for (size_t i = 0; i < local.size(); ++i)
        last_pose_[i] = local[i];
    previous_ = view;
    initialized_ = true;
    sampled_ = false;
    return frame;
}
} // namespace gloom::gameplay
