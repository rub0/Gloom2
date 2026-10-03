#include <gloom/assets/animation.hpp>
#include <assert.h>
#include <math.h>
#include <stdio.h>

using namespace gloom;

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
    uint32 checked = 0;
    for (uint32 node = 0; node < rig.nodes.size(); ++node) {
        if (rig.nodes[node].skin == assets::no_asset_index || rig.nodes[node].mesh == assets::no_asset_index)
            continue;
        const assets::ImportedMesh& mesh = rig.meshes[rig.nodes[node].mesh];
        for (uint32 i = 0; i < mesh.primitive_count; ++i) {
            const assets::ImportedPrimitive& primitive = rig.primitives[mesh.first_primitive + i];
            assets::SkinBounds prepared;
            assets::prepare_skin_bounds(primitive, prepared);
            for (uint32 clip = 0; clip < rig.animations.size(); ++clip) {
                for (uint32 sample = 0; sample <= 60; ++sample) {
                    verify_pose(primitive, prepared,
                        *assets::skin_pose(rig, node,
                            assets::pose_worlds(
                                rig, assets::sample_animation(rig, rig.animations[clip].name, rig.animations[clip].duration * sample / 60.0, false))));
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
    // Exercise eight influences, negative/nonuniform scale, non-unit sums and a zero-weight vertex.
    assets::ImportedPrimitive primitive;
    primitive.vertices.resize(3);
    render::SkinPose pose;
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
