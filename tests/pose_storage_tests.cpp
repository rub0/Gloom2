#include <gloom/gameplay/character_animation.hpp>
#include <gloom/core/allocation_profile.hpp>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NOMINMAX
#include <windows.h>

using namespace gloom;
namespace {
void require(bool ok, const char* message) {
    if (!ok) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}
assets::AnimationRig rig(size_t count, uint64 generation) {
    assets::AnimationRig result;
    result.generation = generation;
    result.names.reserve(6);
    result.names.resize(6);
    memcpy(result.names.data(), "\0move", 6);
    result.nodes.reserve(count);
    result.nodes.resize(count);
    result.rest.reserve(count);
    result.rest.resize(count);
    result.order.reserve(count);
    result.order.resize(count);
    for (uint32 i = 0; i < count; ++i) {
        result.nodes[i].parent = i ? 0 : assets::no_animation_index;
        result.order[i] = i;
    }
    result.nodes[0].skin = 0;
    result.rest[1].position = {1, 0, 0};
    result.skins.reserve(1);
    result.skins.resize(1);
    result.skins[0].joints.reserve(2);
    result.skins[0].joints.resize(2);
    result.skins[0].inverse_bind.reserve(2);
    result.skins[0].inverse_bind.resize(2);
    result.skins[0].joints[0] = 1;
    result.skins[0].joints[1] = 2;
    result.skins[0].inverse_bind[0] = assets::rig_matrix({.position = {-1, 0, 0}});
    result.skins[0].inverse_bind[1] = assets::rig_matrix({});
    result.clips.reserve(1);
    result.clips.resize(1);
    assets::PreparedClip& clip = result.clips[0];
    clip.name = 1;
    clip.name_length = 4;
    clip.duration = 1;
    clip.channels.reserve(2);
    clip.channels.resize(2);
    clip.channels[0].node = 1;
    clip.channels[0].keys.reserve(2);
    clip.channels[0].keys.resize(2);
    clip.channels[0].keys[0] = {.time = 0, .value = {1, 0, 0, 0}};
    clip.channels[0].keys[1] = {.time = 1, .value = {3, 0, 0, 0}};
    clip.channels[1].node = 2;
    clip.channels[1].path = 1;
    clip.channels[1].keys.reserve(2);
    clip.channels[1].keys.resize(2);
    clip.channels[1].keys[0] = {.time = 0, .value = {0, 0, 0, 1}};
    clip.channels[1].keys[1] = {.time = 1, .value = {0, 0, -.707106781F, -.707106781F}};
    return result;
}
struct Actor {
    const assets::AnimationRig* rig;
    gameplay::CharacterAnimator animator;
    gameplay::CombatantView input;
};
DWORD WINAPI evaluate(void* argument) {
    Actor& actor = *static_cast<Actor*>(argument);
    for (uint32 i = 0; i < 1000; ++i) {
        actor.input.aim_pitch = sinf(i * .02F);
        const gameplay::CharacterAnimationFrame& frame = actor.animator.update(*actor.rig, actor.input, 1.0 / 60, false, false, true);
        require(frame.actor == actor.input.entity && !frame.cut, "Concurrent actor identity/history lost");
    }
    return 0;
}
}
int main() {
    assets::AnimationRig initial_rig = rig(4, 1);
    Actor actors[2]{{.rig = &initial_rig, .input = {.entity = 7, .velocity_z = 3}}, {.rig = &initial_rig, .input = {.entity = 8}}};
    for (Actor& actor : actors)
        actor.animator.bind(initial_rig);
    const gameplay::CharacterAnimationFrame& first = actors[0].animator.sample(initial_rig, 7, "move", .25);
    const render::SkinPose* previous = &first.skins[0];
    const Matrix4 saved = previous->matrices[0];
    require(first.cut && fabsf(saved[12] - .5F) < 1e-6F, "Linear sample/bind matrix changed");
    require(fabsf(previous->matrices[1][0] - cosf(.3926990817F)) < 1e-6F, "Quaternion short-arc slerp changed");
    const gameplay::CharacterAnimationFrame& next = actors[0].animator.sample(initial_rig, 7, "move", .75);
    require(!next.cut && &next.skins[0] != previous && previous->matrices[0] == saved, "Previous skin overwritten during current evaluation");
    const gameplay::CharacterAnimationFrame& other = actors[1].animator.sample(initial_rig, 8, "move", .25);
    require(
        &other.skins[0] != previous && &other.skins[0] != &next.skins[0] && previous->matrices[0] == saved, "Actors sharing a rig share mutable skin storage");
    require(fabsf(actors[0].animator.sample(initial_rig, 7, "move", 1.25).skins[0].matrices[0][12] - .5F) < 1e-6F, "Loop wrap changed");
    require(actors[0].animator.sample(initial_rig, 99, "move", .25).cut, "Actor replacement retained motion history");
    assets::LocalPose a, b;
    a.reserve(4);
    a.resize(4);
    b.reserve(4);
    b.resize(4);
    assets::sample_animation(initial_rig, "move", -1, {a.data(), a.size()}, false);
    assets::sample_animation(initial_rig, "move", 2, {b.data(), b.size()}, false);
    assets::blend_poses({a.data(), a.size()}, {b.data(), b.size()}, .5F, {b.data(), b.size()});
    require(fabsf(b[1].position.x - 2) < 1e-6F && fabsf(b[2].rotation.z - sinf(.3926990817F)) < 1e-6F, "Clamping or in-place blend changed");
    initial_rig.clips[0].channels[0].interpolation = 1;
    assets::sample_animation(initial_rig, "move", .75, {a.data(), a.size()});
    require(a[1].position.x == 1, "Step interpolation changed");
    initial_rig.clips[0].channels[0].interpolation = 0;
    initial_rig.clips[0].name_length = 5;
    assets::sample_animation(initial_rig, "move", .75, {a.data(), a.size()});
    require(a[1].position.x == 1, "Embedded zero in clip name aliased a shorter name");
    initial_rig.clips[0].name_length = 4;
    for (Actor& actor : actors) {
        actor.animator.reset();
        actor.animator.update(initial_rig, actor.input, 1.0 / 60, false, false, true);
    }
    allocation_profile_reset();
    allocation_profile_window(true);
    HANDLE workers[2]{};
    for (uint32 i = 0; i < 2; ++i) {
        workers[i] = CreateThread(nullptr, 0, evaluate, &actors[i], 0, nullptr);
        require(workers[i] != nullptr, "Pose worker initialization failed");
    }
    require(WaitForMultipleObjects(2, workers, TRUE, INFINITE) == WAIT_OBJECT_0, "Pose workers did not complete");
    for (HANDLE worker : workers)
        CloseHandle(worker);
    allocation_profile_window(false);
    for (uint32 i = 0; i < static_cast<uint32>(AllocationPhase::count); ++i)
        require(allocation_profile_read(static_cast<AllocationPhase>(i)).calls == 0, "Prepared animation allocated in 1000 evaluations per actor");
    // Rig growth is counted separately; it must invalidate history, then warm to zero allocations again.
    assets::AnimationRig grown_rig = rig(16, 2);
    allocation_profile_reset();
    allocation_profile_window(true);
    {
        AllocationScope phase{AllocationPhase::poses};
        actors[0].animator.bind(grown_rig);
    }
    allocation_profile_window(false);
    require(allocation_profile_read(AllocationPhase::poses).calls > 0, "Growth positive control was not detected");
    require(actors[0].animator.update(grown_rig, actors[0].input, 1.0 / 60).cut, "Rig replacement retained history");
    actors[0].rig = &grown_rig;
    allocation_profile_reset();
    allocation_profile_window(true);
    evaluate(&actors[0]);
    allocation_profile_window(false);
    require(allocation_profile_read(AllocationPhase::poses).calls == 0, "Grown rig failed to stabilize");
    ++grown_rig.generation;
    require(actors[0].animator.update(grown_rig, actors[0].input, 1.0 / 60).cut, "Same-address rig generation retained history");
    ++grown_rig.identity;
    require(actors[0].animator.update(grown_rig, actors[0].input, 1.0 / 60).cut, "Same-address rig lifetime retained history");
    grown_rig.nodes[0].skin = assets::no_animation_index;
    grown_rig.nodes[3].skin = 0;
    ++grown_rig.generation;
    const gameplay::CharacterAnimationFrame& rebound = actors[0].animator.update(grown_rig, actors[0].input, 1.0 / 60);
    require(rebound.cut && rebound.skins[0].matrices.size() == 0 && rebound.skins[3].matrices.size() == 2,
        "Changing skin binding retained a stale palette on a rigid node");
    // All consumers have finished before animator/rig destruction at function exit.
    puts("Pose storage: 2 concurrent actors x 1000 updates, 0 new; previous immutable; growth/generation cut; wrap/clamp/slerp/alias checks passed.");
}
