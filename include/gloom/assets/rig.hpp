#pragma once

#include <gloom/assets/gltf_importer.hpp>
#include <gloom/assets/animation.hpp>
#include <math.h>

namespace gloom::assets {
// Iterative evaluation bounds stack use and rejects cycles/multiple parents.
// Output owns its matrices; a malformed hierarchy leaves it empty.
inline bool bind_node_transforms(const ImportedScene& scene, Array<RigMatrix>& world) {
    world.resize(0);
    Array<uint32> parents, queue;
    parents.reserve(scene.nodes.size());
    parents.resize(scene.nodes.size());
    queue.reserve(scene.nodes.size());
    for (uint32& parent : parents)
        parent = no_asset_index;
    for (uint32 i = 0; i < scene.nodes.size(); ++i)
        for (uint32 child : scene.nodes[i].children) {
            if (child >= scene.nodes.size() || parents[child] != no_asset_index)
                return false;
            parents[child] = i;
        }
    world.reserve(scene.nodes.size());
    world.resize(scene.nodes.size());
    for (uint32 i = 0; i < scene.nodes.size(); ++i)
        if (parents[i] == no_asset_index)
            queue.push_back(i);
    for (size_t head = 0; head < queue.size(); ++head) {
        const uint32 i = queue[head];
        world[i] = parents[i] == no_asset_index ? scene.nodes[i].local_transform : rig_multiply(world[parents[i]], scene.nodes[i].local_transform);
        for (float x : world[i])
            if (!isfinite(x)) {
                world.resize(0);
                return false;
            }
        for (uint32 child : scene.nodes[i].children)
            queue.push_back(child);
    }
    if (queue.size() != scene.nodes.size()) {
        world.resize(0);
        return false;
    }
    return true;
}

inline bool valid_bind_rigs(const ImportedScene& scene) {
    for (const ImportedPrimitive& primitive : scene.primitives)
        for (const ImportedVertex& vertex : primitive.vertices)
            for (float w : vertex.weights)
                if (!isfinite(w) || w < 0 || w > 1)
                    return false;
    Array<RigMatrix> world;
    if (!bind_node_transforms(scene, world))
        return false;
    if (scene.skins.empty()) {
        for (const ImportedNode& node : scene.nodes)
            if (node.skin != no_asset_index)
                return false;
        for (const ImportedPrimitive& primitive : scene.primitives)
            for (const ImportedVertex& vertex : primitive.vertices)
                for (float w : vertex.weights)
                    if (w != 0)
                        return false;
        return true;
    }
    Array<uint32> seen_joints;
    seen_joints.reserve((scene.nodes.size() + 31) / 32);
    seen_joints.resize((scene.nodes.size() + 31) / 32);
    for (const ImportedSkin& skin : scene.skins) {
        if (skin.joints.empty() || skin.joints.size() > 256 || skin.joints.size() != skin.inverse_bind_matrices.size())
            return false;
        for (size_t j = 0; j < skin.joints.size(); ++j) {
            if (skin.joints[j] >= scene.nodes.size())
                return false;
            if (seen_joints[skin.joints[j] / 32] & (1U << (skin.joints[j] % 32)))
                return false;
            seen_joints[skin.joints[j] / 32] |= 1U << (skin.joints[j] % 32);
            for (float x : skin.inverse_bind_matrices[j])
                if (!isfinite(x))
                    return false;
        }
        for (uint32 joint : skin.joints)
            seen_joints[joint / 32] &= ~(1U << (joint % 32));
    }
    for (size_t n = 0; n < scene.nodes.size(); ++n) {
        const ImportedNode& node = scene.nodes[n];
        if (node.skin == no_asset_index) {
            if (node.mesh < scene.meshes.size()) {
                const ImportedMesh& mesh = scene.meshes[node.mesh];
                if (mesh.first_primitive > scene.primitives.size() || mesh.primitive_count > scene.primitives.size() - mesh.first_primitive)
                    return false;
                for (size_t p = mesh.first_primitive; p < mesh.first_primitive + mesh.primitive_count; ++p)
                    for (const ImportedVertex& v : scene.primitives[p].vertices)
                        for (float w : v.weights)
                            if (w != 0)
                                return false;
            }
            continue;
        }
        if (node.skin >= scene.skins.size() || node.mesh >= scene.meshes.size())
            return false;
        const ImportedSkin& skin = scene.skins[node.skin];
        // Milestone 62 displays the authored rest pose. Reject a file whose
        // static vertex positions would differ from evaluating its bind rig.
        for (size_t j = 0; j < skin.joints.size(); ++j) {
            const RigMatrix bound = rig_multiply(world[skin.joints[j]], skin.inverse_bind_matrices[j]);
            for (size_t k = 0; k < 16; ++k)
                if (fabsf(bound[k] - world[n][k]) > 0.001F)
                    return false;
        }
        const ImportedMesh& mesh = scene.meshes[node.mesh];
        if (mesh.first_primitive > scene.primitives.size() || mesh.primitive_count > scene.primitives.size() - mesh.first_primitive)
            return false;
        for (size_t p = mesh.first_primitive; p < mesh.first_primitive + mesh.primitive_count; ++p)
            for (const ImportedVertex& v : scene.primitives[p].vertices) {
                float sum = 0;
                for (size_t i = 0; i < 8; ++i) {
                    if (v.joints[i] >= skin.joints.size())
                        return false;
                    sum += v.weights[i];
                }
                if (fabsf(sum - 1) > 0.0001F)
                    return false;
            }
    }
    return true;
}
} // namespace gloom::assets
