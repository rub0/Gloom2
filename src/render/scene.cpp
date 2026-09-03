#include <gloom/render/scene.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace gloom::render {
namespace {

[[nodiscard]] float lerp(const float first, const float second, const float alpha) noexcept {
    return first + (second - first) * alpha;
}

[[nodiscard]] Vec3 lerp(const Vec3 first, const Vec3 second, const float alpha) noexcept {
    return {
        lerp(first.x, second.x, alpha),
        lerp(first.y, second.y, alpha),
        lerp(first.z, second.z, alpha),
    };
}

} // namespace

Transform normal_transform(const Transform& transform) noexcept {
    const auto inverse = [](float value) { return std::abs(value) > 1.0e-8F ? 1.0F / value : 0.0F; };
    return {.rotation = transform.rotation,
            .scale = {inverse(transform.scale.x), inverse(transform.scale.y), inverse(transform.scale.z)}};
}

Transform attach_transform(const Transform& parent, const Transform& local) noexcept {
    const auto& a=parent.rotation;const auto& b=local.rotation;
    const Vec3 v{local.position.x*parent.scale.x,local.position.y*parent.scale.y,local.position.z*parent.scale.z};
    const Vec3 t{2*(a.y*v.z-a.z*v.y),2*(a.z*v.x-a.x*v.z),2*(a.x*v.y-a.y*v.x)};
    return {.position={parent.position.x+v.x+a.w*t.x+a.y*t.z-a.z*t.y,
                       parent.position.y+v.y+a.w*t.y+a.z*t.x-a.x*t.z,
                       parent.position.z+v.z+a.w*t.z+a.x*t.y-a.y*t.x},
            .rotation={a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
                       a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
                       a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
                       a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z},
            .scale={parent.scale.x*local.scale.x,parent.scale.y*local.scale.y,parent.scale.z*local.scale.z}};
}

Transform camera_relative_transform(const Camera& camera, Vec3 offset, Vec3 scale) {
    const auto cross = [](Vec3 a, Vec3 b) { return Vec3{a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; };
    const auto normalize = [](Vec3 v) {
        const float length = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
        if (!std::isfinite(length) || length < 1.0e-6F) throw std::invalid_argument{"Invalid view model camera basis"};
        return Vec3{v.x/length, v.y/length, v.z/length};
    };
    const auto f = normalize({camera.target.x-camera.position.x, camera.target.y-camera.position.y,
                              camera.target.z-camera.position.z});
    const auto r = normalize(cross(camera.up, f));
    const auto u = cross(f, r);
    Quaternion q;
    const float trace = r.x + u.y + f.z;
    if (trace > 0) {
        const float s = std::sqrt(trace + 1.0F) * 2.0F;
        q = {(u.z-f.y)/s, (f.x-r.z)/s, (r.y-u.x)/s, s*0.25F};
    } else if (r.x > u.y && r.x > f.z) {
        const float s = std::sqrt(1.0F + r.x-u.y-f.z)*2.0F;
        q = {s*0.25F, (u.x+r.y)/s, (f.x+r.z)/s, (u.z-f.y)/s};
    } else if (u.y > f.z) {
        const float s = std::sqrt(1.0F + u.y-r.x-f.z)*2.0F;
        q = {(u.x+r.y)/s, s*0.25F, (f.y+u.z)/s, (f.x-r.z)/s};
    } else {
        const float s = std::sqrt(1.0F + f.z-r.x-u.y)*2.0F;
        q = {(f.x+r.z)/s, (f.y+u.z)/s, s*0.25F, (r.y-u.x)/s};
    }
    return {.position = {camera.position.x + r.x*offset.x + u.x*offset.y + f.x*offset.z,
                          camera.position.y + r.y*offset.x + u.y*offset.y + f.y*offset.z,
                          camera.position.z + r.z*offset.x + u.z*offset.y + f.z*offset.z},
            .rotation = q, .scale = scale};
}

Transform interpolate(const Transform& previous, const Transform& current, const float alpha) noexcept {
    const float amount = std::clamp(alpha, 0.0F, 1.0F);
    Quaternion end = current.rotation;
    const float dot = previous.rotation.x * end.x + previous.rotation.y * end.y +
                      previous.rotation.z * end.z + previous.rotation.w * end.w;
    if (dot < 0.0F) {
        end = {-end.x, -end.y, -end.z, -end.w};
    }

    Quaternion rotation{
        lerp(previous.rotation.x, end.x, amount),
        lerp(previous.rotation.y, end.y, amount),
        lerp(previous.rotation.z, end.z, amount),
        lerp(previous.rotation.w, end.w, amount),
    };
    const float length = std::sqrt(rotation.x * rotation.x + rotation.y * rotation.y +
                                   rotation.z * rotation.z + rotation.w * rotation.w);
    if (length > 0.0F) {
        rotation.x /= length;
        rotation.y /= length;
        rotation.z /= length;
        rotation.w /= length;
    } else {
        rotation = {};
    }

    return {
        .position = lerp(previous.position, current.position, amount),
        .rotation = rotation,
        .scale = lerp(previous.scale, current.scale, amount),
    };
}

} // namespace gloom::render
