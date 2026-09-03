#include <gloom/render/visibility.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace gloom::render {
namespace {

[[nodiscard]] Vec3 add(const Vec3 left, const Vec3 right) noexcept {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}
[[nodiscard]] Vec3 subtract(const Vec3 left, const Vec3 right) noexcept {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}
[[nodiscard]] Vec3 multiply(const Vec3 value, const float scale) noexcept {
    return {value.x * scale, value.y * scale, value.z * scale};
}
[[nodiscard]] float dot(const Vec3 left, const Vec3 right) noexcept {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}
[[nodiscard]] Vec3 cross(const Vec3 left, const Vec3 right) noexcept {
    return {left.y * right.z - left.z * right.y,
            left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}
[[nodiscard]] float length(const Vec3 value) noexcept { return std::sqrt(dot(value, value)); }
[[nodiscard]] Vec3 normalize(const Vec3 value) noexcept {
    const float magnitude = length(value);
    return magnitude > 1.0e-7F ? multiply(value, 1.0F / magnitude) : Vec3{};
}
[[nodiscard]] Vec3 rotate(const Quaternion rotation, const Vec3 value) noexcept {
    const Vec3 q{rotation.x, rotation.y, rotation.z};
    const Vec3 twice_cross = multiply(cross(q, value), 2.0F);
    return add(value, add(multiply(twice_cross, rotation.w), cross(q, twice_cross)));
}

struct WorldSphere {
    Vec3 center;
    float radius;
};

[[nodiscard]] WorldSphere world_sphere(const RenderInstance& instance) noexcept {
    const Vec3 scaled{instance.local_bounds.center.x * instance.transform.scale.x,
                      instance.local_bounds.center.y * instance.transform.scale.y,
                      instance.local_bounds.center.z * instance.transform.scale.z};
    const float maximum_scale = std::max({std::abs(instance.transform.scale.x),
                                          std::abs(instance.transform.scale.y),
                                          std::abs(instance.transform.scale.z)});
    return {.center = add(instance.transform.position, rotate(instance.transform.rotation, scaled)),
            .radius = instance.local_bounds.radius * maximum_scale};
}

struct Frustum {
    Vec3 position;
    Vec3 right;
    Vec3 up;
    Vec3 forward;
    float near_plane;
    float far_plane;
    float tangent_x;
    float tangent_y;
};

[[nodiscard]] bool visible(const Frustum& frustum, const WorldSphere sphere) noexcept {
    const Vec3 relative = subtract(sphere.center, frustum.position);
    const float depth = dot(relative, frustum.forward);
    if (depth + sphere.radius < frustum.near_plane ||
        depth - sphere.radius > frustum.far_plane) {
        return false;
    }
    const float horizontal = dot(relative, frustum.right);
    const float vertical = dot(relative, frustum.up);
    const float horizontal_radius = sphere.radius * std::sqrt(1.0F +
                                                               frustum.tangent_x *
                                                                   frustum.tangent_x);
    const float vertical_radius = sphere.radius * std::sqrt(1.0F +
                                                             frustum.tangent_y *
                                                                 frustum.tangent_y);
    return std::abs(horizontal) <= depth * frustum.tangent_x + horizontal_radius &&
           std::abs(vertical) <= depth * frustum.tangent_y + vertical_radius;
}

} // namespace

RenderSnapshot PreparedVisibility::snapshot() const noexcept {
    return {.camera = camera, .instances = instances, .batches = batches};
}

VisibilitySystem::VisibilitySystem(core::JobSystem& jobs, const VisibilitySettings settings)
    : jobs_{jobs}, settings_{settings} {
    if (settings_.instances_per_job == 0 || settings_.lod1_distance_in_radii <= 0.0F ||
        settings_.lod2_distance_in_radii <= settings_.lod1_distance_in_radii) {
        throw std::invalid_argument{"Visibility settings are invalid"};
    }
}

PreparedVisibility VisibilitySystem::build(const Camera& camera,
                                           const float aspect_ratio,
                                           const std::span<const RenderInstance> instances) {
    const auto started = std::chrono::steady_clock::now();
    if (!(aspect_ratio > 0.0F) || !std::isfinite(aspect_ratio)) {
        throw std::invalid_argument{"Visibility aspect ratio must be positive"};
    }
    const Vec3 forward = normalize(subtract(camera.target, camera.position));
    const Vec3 right = normalize(cross(camera.up, forward));
    const Vec3 up = cross(forward, right);
    const float tangent_y = std::tan(camera.vertical_field_of_view_radians * 0.5F);
    const Frustum frustum{.position = camera.position,
                          .right = right,
                          .up = up,
                          .forward = forward,
                          .near_plane = camera.near_plane,
                          .far_plane = camera.far_plane,
                          .tangent_x = tangent_y * aspect_ratio,
                          .tangent_y = tangent_y};

    const std::size_t job_count =
        (instances.size() + settings_.instances_per_job - 1U) / settings_.instances_per_job;
    std::vector<std::vector<RenderInstance>> visible_by_job(job_count);
    std::vector<std::array<std::uint64_t, 3>> lod_by_job(job_count);
    auto group = jobs_.create_group();
    for (std::size_t job = 0; job < job_count; ++job) {
        const std::size_t begin = job * settings_.instances_per_job;
        const std::size_t end = std::min(begin + settings_.instances_per_job, instances.size());
        jobs_.schedule(group, [&, job, begin, end] {
            auto& output = visible_by_job[job];
            output.reserve(end - begin);
            for (std::size_t index = begin; index < end; ++index) {
                RenderInstance instance = instances[index];
                const auto sphere = world_sphere(instance);
                if (!visible(frustum, sphere)) {
                    continue;
                }
                const float distance_in_radii =
                    length(subtract(sphere.center, camera.position)) /
                    std::max(sphere.radius, 1.0e-4F);
                std::size_t lod = 0;
                if (instance.lod_count > 2 &&
                    distance_in_radii >= settings_.lod2_distance_in_radii) {
                    lod = 2;
                } else if (instance.lod_count > 1 &&
                           distance_in_radii >= settings_.lod1_distance_in_radii) {
                    lod = 1;
                }
                if (instance.lod_meshes[lod].value != 0) {
                    instance.mesh = instance.lod_meshes[lod];
                }
                ++lod_by_job[job][lod];
                output.push_back(instance);
            }
        });
    }
    if (job_count != 0) {
        jobs_.wait(group);
    }

    PreparedVisibility result{.camera = camera};
    result.metrics.submitted_instances = instances.size();
    result.metrics.culling_jobs = job_count;
    for (std::size_t job = 0; job < job_count; ++job) {
        result.instances.insert(result.instances.end(),
                                visible_by_job[job].begin(),
                                visible_by_job[job].end());
        for (std::size_t lod = 0; lod < 3; ++lod) {
            result.metrics.lod_instances[lod] += lod_by_job[job][lod];
        }
    }
    std::ranges::sort(result.instances, [](const RenderInstance& left, const RenderInstance& right) {
        return left.material.value < right.material.value ||
               (left.material == right.material && left.mesh.value < right.mesh.value);
    });
    for (std::size_t first = 0; first < result.instances.size();) {
        std::size_t end = first + 1U;
        while (end < result.instances.size() &&
               result.instances[end].mesh == result.instances[first].mesh &&
               result.instances[end].material == result.instances[first].material) {
            ++end;
        }
        result.batches.push_back({.mesh = result.instances[first].mesh,
                                  .material = result.instances[first].material,
                                  .first_instance = static_cast<std::uint32_t>(first),
                                  .instance_count = static_cast<std::uint32_t>(end - first)});
        first = end;
    }
    result.metrics.visible_instances = result.instances.size();
    result.metrics.culled_instances = instances.size() - result.instances.size();
    result.metrics.batches = result.batches.size();
    result.metrics.build_nanoseconds = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - started)
            .count());
    return result;
}

} // namespace gloom::render
