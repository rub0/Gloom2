#include <gloom/render/visibility.hpp>

#include <assert.h>
#include <math.h>
#include <stdlib.h>

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
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z, left.x * right.y - left.y * right.x};
}
[[nodiscard]] float length(const Vec3 value) noexcept {
    return sqrtf(dot(value, value));
}
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
    const Vec3 scaled{instance.local_bounds.center.x * instance.transform.scale.x, instance.local_bounds.center.y * instance.transform.scale.y,
        instance.local_bounds.center.z * instance.transform.scale.z};
    const float maximum_scale = fmaxf(fabsf(instance.transform.scale.x), fmaxf(fabsf(instance.transform.scale.y), fabsf(instance.transform.scale.z)));
    return {.center = add(instance.transform.position, rotate(instance.transform.rotation, scaled)), .radius = instance.local_bounds.radius * maximum_scale};
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
    if (depth + sphere.radius < frustum.near_plane || depth - sphere.radius > frustum.far_plane) {
        return false;
    }
    const float horizontal = dot(relative, frustum.right);
    const float vertical = dot(relative, frustum.up);
    const float horizontal_radius = sphere.radius * sqrtf(1.0F + frustum.tangent_x * frustum.tangent_x);
    const float vertical_radius = sphere.radius * sqrtf(1.0F + frustum.tangent_y * frustum.tangent_y);
    return fabsf(horizontal) <= depth * frustum.tangent_x + horizontal_radius && fabsf(vertical) <= depth * frustum.tangent_y + vertical_radius;
}

} // namespace

RenderSnapshot PreparedVisibility::snapshot() const noexcept {
    return {.camera = camera, .instances = {instances.data(), instances.size()}, .batches = {batches.data(), batches.size()}};
}

VisibilitySystem::VisibilitySystem(core::JobSystem& jobs, const VisibilitySettings settings) : jobs_{jobs}, settings_{settings} {
    assert(settings_.instances_per_job > 0 && settings_.lod1_distance_in_radii > 0.0F && settings_.lod2_distance_in_radii > settings_.lod1_distance_in_radii);
}

int VisibilitySystem::compare_indices(const void* left, const void* right) {
    const VisibleIndex& a = *static_cast<const VisibleIndex*>(left);
    const VisibleIndex& b = *static_cast<const VisibleIndex*>(right);
    if (a.material.value != b.material.value)
        return a.material.value < b.material.value ? -1 : 1;
    if (a.mesh.value != b.mesh.value)
        return a.mesh.value < b.mesh.value ? -1 : 1;
    return a.source < b.source ? -1 : a.source > b.source ? 1 : 0;
}

void VisibilitySystem::build(PreparedVisibility& result, const Camera& camera, const float aspect_ratio, Span<const RenderInstance> instances) {
    const uint64 started = performance_clock();
    assert(aspect_ratio > 0.0F && isfinite(aspect_ratio));
    assert(instances.empty() ||
           reinterpret_cast<uint64>(instances.values) >=
               reinterpret_cast<uint64>(result.instances.data()) + result.instances.capacity() * sizeof(RenderInstance) ||
           reinterpret_cast<uint64>(instances.values) + instances.size() * sizeof(RenderInstance) <= reinterpret_cast<uint64>(result.instances.data()));
    const Vec3 forward = normalize(subtract(camera.target, camera.position));
    const Vec3 right = normalize(cross(camera.up, forward));
    const Vec3 up = cross(forward, right);
    const float tangent_y = tanf(camera.vertical_field_of_view_radians * 0.5F);
    const Frustum frustum{.position = camera.position,
        .right = right,
        .up = up,
        .forward = forward,
        .near_plane = camera.near_plane,
        .far_plane = camera.far_plane,
        .tangent_x = tangent_y * aspect_ratio,
        .tangent_y = tangent_y};
    const size_t job_count = (instances.size() + settings_.instances_per_job - 1U) / settings_.instances_per_job;
    visible_indices_.reserve(instances.size());
    visible_indices_.resize(instances.size());
    job_outputs_.reserve(job_count);
    job_outputs_.resize(job_count);
    result.camera = camera;
    result.metrics = {.submitted_instances = instances.size(), .culling_jobs = job_count};

    struct CullJob {
        const Frustum* frustum;
        Span<const RenderInstance> instances;
        VisibleIndex* indices;
        JobOutput* output;
        VisibilitySettings settings;
        size_t begin, end;

        void operator()() const {
            *output = {};
            for (size_t index = begin; index < end; ++index) {
                const RenderInstance& instance = instances[index];
                const WorldSphere sphere = world_sphere(instance);
                if (!visible(*frustum, sphere))
                    continue;
                const float distance_in_radii = length(subtract(sphere.center, frustum->position)) / fmaxf(sphere.radius, 1.0e-4F);
                uint32 lod = 0;
                if (instance.lod_count > 2 && distance_in_radii >= settings.lod2_distance_in_radii)
                    lod = 2;
                else if (instance.lod_count > 1 && distance_in_radii >= settings.lod1_distance_in_radii)
                    lod = 1;
                indices[begin + output->count++] = {
                    .source = index, .mesh = instance.lod_meshes[lod].value ? instance.lod_meshes[lod] : instance.mesh, .material = instance.material};
                ++output->lod_instances[lod];
            }
        }
    };

    if (job_count) {
        core::TaskGroup group = jobs_.create_group();
        for (size_t job = 0; job < job_count; ++job) {
            const size_t begin = job * settings_.instances_per_job;
            jobs_.schedule(group, CullJob{.frustum = &frustum,
                                      .instances = instances,
                                      .indices = visible_indices_.data(),
                                      .output = &job_outputs_[job],
                                      .settings = settings_,
                                      .begin = begin,
                                      .end = instances.size() - begin < settings_.instances_per_job ? instances.size() : begin + settings_.instances_per_job});
        }
        jobs_.wait(group);
    }
    size_t visible_count = 0;
    for (size_t job = 0; job < job_count; ++job) {
        for (size_t index = 0; index < job_outputs_[job].count; ++index)
            visible_indices_[visible_count++] = visible_indices_[job * settings_.instances_per_job + index];
        for (uint32 lod = 0; lod < 3; ++lod)
            result.metrics.lod_instances[lod] += job_outputs_[job].lod_instances[lod];
    }
    if (visible_count > 1)
        qsort(visible_indices_.data(), visible_count, sizeof(VisibleIndex), compare_indices);
    result.instances.reserve(visible_count);
    result.instances.resize(visible_count);
    result.batches.reserve(visible_count);
    result.batches.resize(visible_count);
    size_t batch_count = 0;
    for (size_t index = 0; index < visible_count; ++index) {
        const VisibleIndex& selected = visible_indices_[index];
        result.instances[index] = instances[selected.source];
        result.instances[index].mesh = selected.mesh;
        if (index == 0 || selected.mesh != visible_indices_[index - 1].mesh || selected.material != visible_indices_[index - 1].material)
            result.batches[batch_count++] = {.mesh = selected.mesh, .material = selected.material, .first_instance = static_cast<uint32>(index)};
        ++result.batches[batch_count - 1].instance_count;
    }
    result.batches.resize(batch_count);
    result.metrics.visible_instances = visible_count;
    result.metrics.culled_instances = instances.size() - visible_count;
    result.metrics.batches = batch_count;
    result.metrics.build_nanoseconds = (performance_clock() - started) * 1000000000ULL / clock_frequency_;
}

} // namespace gloom::render
