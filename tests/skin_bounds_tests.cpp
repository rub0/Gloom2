#include <gloom/assets/animation.hpp>
#include <gloom/assets/gltf_importer.hpp>
#include <gloom/gameplay/character_animation.hpp>
#include <gloom/core/allocation_profile.hpp>
#include <assert.h>
#include <math.h>
#include <stdio.h>

using namespace gloom;

void verify_warm(const assets::AnimationRig& rig) {
    gameplay::CharacterAnimator actors[2];
    gameplay::CombatantView inputs[2]{{.entity = 7, .velocity_z = 3}, {.entity = 8, .velocity_x = 2}};
    for (uint32 i = 0; i < 2; ++i) {
        actors[i].bind(rig);
        actors[i].update(rig, inputs[i], 1.0 / 60, false, false, i == 1);
    }
#ifdef GLOOM_ALLOCATION_PROFILE
    allocation_profile_reset();
    allocation_profile_window(true);
#endif
    {
        GLOOM_PROFILE_SCOPE(AllocationPhase::poses);
        for (uint32 frame = 0; frame < 1000; ++frame)
            for (uint32 actor = 0; actor < 2; ++actor) {
                inputs[actor].aim_pitch = sinf(frame * .01F);
                const gameplay::CharacterAnimationFrame& evaluated = actors[actor].update(rig, inputs[actor], 1.0 / 60, false, false, actor == 1);
                assert(!evaluated.cut && evaluated.actor == inputs[actor].entity && isfinite(evaluated.worlds[0][13]));
            }
    }
#ifdef GLOOM_ALLOCATION_PROFILE
    allocation_profile_window(false);
    assert(allocation_profile_read(AllocationPhase::poses).calls == 0);
    printf("Real rig: %llu nodes, 2 actors x 1000 evaluations, 0 new\n", static_cast<unsigned long long>(rig.nodes.size()));
#endif
}

void verify_pose(const assets::ImportedPrimitive& primitive, const assets::SkinBounds& prepared, const render::SkinPose& pose) {
    const render::BoundingSphere reference = assets::skinned_bounds(primitive, pose);
    const render::BoundingSphere bounds = assets::skinned_bounds(prepared, pose);
    for (const assets::ImportedVertex& vertex : primitive.vertices) {
        const render::Vec3 point = assets::skinned_position(vertex, pose);
        assert(hypotf(hypotf(point.x - reference.center.x, point.y - reference.center.y), point.z - reference.center.z) <= reference.radius + .001F);
        assert(hypotf(hypotf(point.x - bounds.center.x, point.y - bounds.center.y), point.z - bounds.center.z) <= bounds.radius + .001F);
    }
}

void verify_rig(const assets::ImportedScene& rig) {
    assets::AnimationRig runtime;
    assert(assets::prepare_animation_rig(rig, runtime));
    verify_warm(runtime);
    assets::LocalPose local;
    local.reserve(runtime.nodes.size());
    local.resize(runtime.nodes.size());
    Array<assets::RigMatrix> worlds;
    worlds.reserve(local.size());
    worlds.resize(local.size());
    uint32 checked = 0;
    for (uint32 node = 0; node < rig.nodes.size(); ++node) {
        if (rig.nodes[node].skin == assets::no_asset_index || rig.nodes[node].mesh == assets::no_asset_index)
            continue;
        const assets::ImportedMesh& mesh = rig.meshes[rig.nodes[node].mesh];
        render::SkinPose pose;
        assets::prepare_skin_pose(runtime, node, pose);
        for (uint32 i = 0; i < mesh.primitive_count; ++i) {
            const assets::ImportedPrimitive& primitive = rig.primitives[mesh.first_primitive + i];
            assets::SkinBounds prepared;
            assets::prepare_skin_bounds(primitive, prepared);
            for (uint32 clip = 0; clip < rig.animations.size(); ++clip) {
                for (uint32 sample = 0; sample <= 60; ++sample) {
                    assets::sample_animation(
                        runtime, rig.animations[clip].name.c_str(), rig.animations[clip].duration * sample / 60.0, {local.data(), local.size()}, false);
                    assets::pose_worlds(runtime, {local.data(), local.size()}, {worlds.data(), worlds.size()});
                    assets::skin_pose(runtime, node, {worlds.data(), worlds.size()}, pose);
                    verify_pose(primitive, prepared, pose);
                    ++checked;
                }
            }
        }
    }
    assert(checked);
    printf("Skin bounds: %u primitive/pose comparisons against reference vertices\n", checked);
}

int main() {
    verify_rig(*assets::import_gltf(GLOOM_TEST_ASSETS "/characters/hound_rig/v16/hound-rig.gltf"));
    verify_rig(*assets::import_gltf(GLOOM_TEST_ASSETS "/characters/original/archangel.gltf"));
    assets::AnimationRig shadow;
    assert(assets::prepare_animation_rig(*assets::import_gltf(GLOOM_TEST_ASSETS "/characters/original/shadow.gltf"), shadow));
    verify_warm(shadow);
    assets::AnimationRig hound_production;
    assert(assets::prepare_animation_rig(*assets::import_gltf(GLOOM_TEST_ASSETS "/characters/hound_rig/v17/hound-rig.gltf"), hound_production));
    verify_warm(hound_production);
    assets::ImportedScene invalid = *assets::import_gltf(GLOOM_TEST_ASSETS "/characters/original/archangel.gltf");
    assets::AnimationRig replaced;
    assert(assets::prepare_animation_rig(invalid, replaced));
    const uint64 previous_identity = replaced.identity;
    assert(assets::prepare_animation_rig(invalid, replaced) && replaced.identity != previous_identity);
    const uint64 valid_identity = replaced.identity;
    invalid.nodes[0].children.resize(1);
    invalid.nodes[0].children[0] = 0;
    assert(!assets::prepare_animation_rig(invalid, replaced) && replaced.identity == valid_identity);
    invalid = *assets::import_gltf(GLOOM_TEST_ASSETS "/characters/original/archangel.gltf");
    invalid.primitives[0].vertices[0].position[0] = NAN;
    assert(!assets::prepare_animation_rig(invalid, replaced) && replaced.identity == valid_identity);
    invalid.primitives[0].vertices[0].position[0] = 0;
    invalid.nodes[0].local_transform[0] = 0;
    invalid.nodes[0].local_transform[1] = 0;
    invalid.nodes[0].local_transform[2] = 0;
    assert(!assets::prepare_animation_rig(invalid, replaced) && replaced.identity == valid_identity);
    // Exercise eight influences, negative/nonuniform scale, non-unit sums and a zero-weight vertex.
    assets::ImportedPrimitive primitive;
    primitive.vertices.resize(3);
    render::SkinPose pose;
    pose.matrices.reserve(8);
    pose.matrices.resize(8);
    for (uint32 i = 0; i < 8; ++i) {
        primitive.vertices[0].position = {-2, 1, 3};
        primitive.vertices[1].position = {4, -3, -1};
        primitive.vertices[0].joints[i] = primitive.vertices[1].joints[i] = static_cast<uint16>(i);
        primitive.vertices[0].weights[i] = .1F;
        primitive.vertices[1].weights[i] = .15F;
        pose.matrices[i] =
            assets::rig_matrix({.position = {static_cast<float>(i) - 5, -2, 3}, .rotation = {0, sinf(i * .2F), 0, cosf(i * .2F)}, .scale = {-1, 2, .5F}});
    }
    assets::SkinBounds prepared;
    assets::prepare_skin_bounds(primitive, prepared);
    verify_pose(primitive, prepared, pose);
    puts("Skin bounds synthetic checks passed");
}
