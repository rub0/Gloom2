#pragma once
#include <gloom/physics/world.hpp>
#include <algorithm>
#include <cmath>

namespace gloom::physics {
inline float ray_triangle_distance(const TriangleMesh& mesh, Vec3 origin,
                                   Vec3 direction, float maximum) noexcept {
    const auto sub=[](Vec3 a,Vec3 b){return Vec3{a.x-b.x,a.y-b.y,a.z-b.z};};
    const auto dot=[](Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;};
    const auto cross=[](Vec3 a,Vec3 b){return Vec3{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
    for (std::size_t i=0;i+2<mesh.indices.size();i+=3) {
        const auto a=mesh.vertices[mesh.indices[i]];
        const auto e1=sub(mesh.vertices[mesh.indices[i+1]],a);
        const auto e2=sub(mesh.vertices[mesh.indices[i+2]],a);
        const auto h=cross(direction,e2);
        const float determinant=dot(e1,h);
        if (std::abs(determinant)<1e-7F) continue;
        const float inverse=1.0F/determinant;
        const auto relative=sub(origin,a);
        const float u=dot(relative,h)*inverse;
        if (u<0 || u>1) continue;
        const auto q=cross(relative,e1);
        const float v=dot(direction,q)*inverse;
        if (v<0 || u+v>1) continue;
        const float distance=dot(e2,q)*inverse;
        if (distance>=0 && distance<maximum) maximum=distance;
    }
    return maximum;
}
}
