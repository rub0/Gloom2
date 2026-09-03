#pragma once

#include <gloom/assets/gltf_importer.hpp>
#include <algorithm>
#include <cmath>
#include <optional>

namespace gloom::assets {
using RigMatrix = std::array<float, 16>;

inline RigMatrix rig_multiply(const RigMatrix& a, const RigMatrix& b) {
    RigMatrix result{};
    for (std::size_t c=0;c<4;++c) for (std::size_t r=0;r<4;++r)
        for (std::size_t k=0;k<4;++k) result[c*4+r]+=a[k*4+r]*b[c*4+k];
    return result;
}

// Iterative evaluation bounds stack use and rejects cycles/multiple parents.
inline std::optional<std::vector<RigMatrix>> bind_node_transforms(const ImportedScene& scene) {
    std::vector<std::uint32_t> parents(scene.nodes.size(), no_asset_index), queue;
    for (std::uint32_t i=0;i<scene.nodes.size();++i) for (auto child:scene.nodes[i].children) {
        if (child>=scene.nodes.size() || parents[child]!=no_asset_index) return {};
        parents[child]=i;
    }
    std::vector<RigMatrix> world(scene.nodes.size());
    for (std::uint32_t i=0;i<scene.nodes.size();++i) if (parents[i]==no_asset_index) queue.push_back(i);
    for (std::size_t head=0;head<queue.size();++head) {
        const auto i=queue[head];
        world[i]=parents[i]==no_asset_index ? scene.nodes[i].local_transform
            : rig_multiply(world[parents[i]],scene.nodes[i].local_transform);
        if (!std::ranges::all_of(world[i],[](float x){return std::isfinite(x);})) return {};
        queue.insert(queue.end(),scene.nodes[i].children.begin(),scene.nodes[i].children.end());
    }
    if (queue.size()!=scene.nodes.size()) return {};
    return world;
}

inline bool valid_bind_rigs(const ImportedScene& scene) {
    for (const auto& primitive:scene.primitives) for (const auto& vertex:primitive.vertices)
        for (float w:vertex.weights) if (!std::isfinite(w) || w<0 || w>1) return false;
    if (scene.skins.empty()) {
        for (const auto& node:scene.nodes) if (node.skin!=no_asset_index) return false;
        for (const auto& primitive:scene.primitives) for (const auto& vertex:primitive.vertices)
            for (float w:vertex.weights) if (w!=0) return false;
        return true;
    }
    const auto world=bind_node_transforms(scene);
    if (!world) return false;
    for (const auto& skin:scene.skins) {
        if (skin.joints.empty() || skin.joints.size()>256 || skin.joints.size()!=skin.inverse_bind_matrices.size()) return false;
        auto sorted=skin.joints;std::ranges::sort(sorted);
        if (std::adjacent_find(sorted.begin(),sorted.end())!=sorted.end()) return false;
        for (std::size_t j=0;j<skin.joints.size();++j)
            if (skin.joints[j]>=scene.nodes.size() || !std::ranges::all_of(skin.inverse_bind_matrices[j],[](float x){return std::isfinite(x);})) return false;
    }
    for (std::size_t n=0;n<scene.nodes.size();++n) {
        const auto& node=scene.nodes[n];
        if (node.skin==no_asset_index) {
            if (node.mesh<scene.meshes.size()) {
                const auto& mesh=scene.meshes[node.mesh];
                if (mesh.first_primitive>scene.primitives.size() || mesh.primitive_count>scene.primitives.size()-mesh.first_primitive) return false;
                for (std::size_t p=mesh.first_primitive;p<mesh.first_primitive+mesh.primitive_count;++p)
                    for (const auto& v:scene.primitives[p].vertices) for (float w:v.weights) if (w!=0) return false;
            }
            continue;
        }
        if (node.skin>=scene.skins.size() || node.mesh>=scene.meshes.size()) return false;
        const auto& skin=scene.skins[node.skin];
        // Milestone 62 displays the authored rest pose. Reject a file whose
        // static vertex positions would differ from evaluating its bind rig.
        for (std::size_t j=0;j<skin.joints.size();++j) {
            const auto bound=rig_multiply((*world)[skin.joints[j]],skin.inverse_bind_matrices[j]);
            for (std::size_t k=0;k<16;++k)
                if (std::abs(bound[k]-(*world)[n][k])>0.001F) return false;
        }
        const auto& mesh=scene.meshes[node.mesh];
        if (mesh.first_primitive>scene.primitives.size() || mesh.primitive_count>scene.primitives.size()-mesh.first_primitive) return false;
        for (std::size_t p=mesh.first_primitive;p<mesh.first_primitive+mesh.primitive_count;++p)
            for (const auto& v:scene.primitives[p].vertices) {
                float sum=0;
                for (std::size_t i=0;i<8;++i) { if (v.joints[i]>=skin.joints.size()) return false;sum+=v.weights[i]; }
                if (std::abs(sum-1)>0.0001F) return false;
            }
    }
    return true;
}
} // namespace gloom::assets
