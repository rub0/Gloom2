#include <gloom/render/lighting.hpp>

#include <assert.h>
#include <math.h>

namespace gloom::render {
namespace {

[[nodiscard]] Vec3 subtract(const Vec3 left, const Vec3 right) noexcept {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}
[[nodiscard]] float dot(const Vec3 left, const Vec3 right) noexcept {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}
[[nodiscard]] Vec3 cross(const Vec3 left, const Vec3 right) noexcept {
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z, left.x * right.y - left.y * right.x};
}
[[nodiscard]] Vec3 normalize(const Vec3 value) noexcept {
    const float magnitude = sqrtf(dot(value, value));
    return magnitude > 1.0e-7F ? Vec3{value.x / magnitude, value.y / magnitude, value.z / magnitude} : Vec3{};
}

} // namespace

ClusteredLightingView PreparedLighting::view() const noexcept {
    return {.directional = directional,
        .environment = environment,
        .grid = grid,
        .point_lights = {point_lights.data(), point_lights.size()},
        .clusters = {clusters.data(), clusters.size()},
        .light_indices = {light_indices.data(), light_indices.size()}};
}

ClusteredLightingBuilder::ClusteredLightingBuilder(const LightingSettings settings) : settings_{settings} {
    assert(settings_.grid_width && settings_.grid_height && settings_.grid_depth && settings_.maximum_lights && settings_.maximum_lights_per_cluster);
    light_bounds_.reserve(settings_.maximum_lights);
    light_bounds_.resize(settings_.maximum_lights);
}

void ClusteredLightingBuilder::build(PreparedLighting& result, const Camera& camera, const float aspect_ratio, Span<const PointLight> lights,
    const DirectionalLight& directional, const EnvironmentLighting& environment) {
    const uint64 started = performance_clock();
    assert(aspect_ratio > 0.0F && isfinite(aspect_ratio) && camera.near_plane > 0.0F && camera.far_plane > camera.near_plane);
    assert(
        lights.empty() ||
        reinterpret_cast<uint64>(lights.values) >= reinterpret_cast<uint64>(result.point_lights.data()) + result.point_lights.capacity() * sizeof(PointLight) ||
        reinterpret_cast<uint64>(lights.values) + lights.size() * sizeof(PointLight) <= reinterpret_cast<uint64>(result.point_lights.data()));
    result.directional = directional;
    result.environment = environment;
    result.grid = {.width = settings_.grid_width,
        .height = settings_.grid_height,
        .depth = settings_.grid_depth,
        .near_plane = camera.near_plane,
        .far_plane = camera.far_plane};
    result.metrics = {.input_lights = lights.size()};
    const size_t cluster_count = static_cast<size_t>(settings_.grid_width) * settings_.grid_height * settings_.grid_depth;
    result.point_lights.reserve(settings_.maximum_lights);
    result.point_lights.resize(settings_.maximum_lights);
    result.clusters.reserve(cluster_count);
    result.clusters.resize(cluster_count);
    for (LightClusterRange& cluster : result.clusters)
        cluster = {};
    size_t active_lights = 0;
    for (const PointLight& light : lights) {
        if (active_lights == settings_.maximum_lights)
            break;
        if (light.range > 0.0F && light.intensity > 0.0F && isfinite(light.range) && isfinite(light.intensity))
            result.point_lights[active_lights++] = light;
    }
    result.point_lights.resize(active_lights);
    result.metrics.active_lights = active_lights;
    const Vec3 forward = normalize(subtract(camera.target, camera.position));
    const Vec3 right = normalize(cross(camera.up, forward));
    const Vec3 up = cross(forward, right);
    const float tangent_y = tanf(camera.vertical_field_of_view_radians * 0.5F);
    const float tangent_x = tangent_y * aspect_ratio;
    const float log_range = logf(camera.far_plane / camera.near_plane);
    for (uint32 light_index = 0; light_index < active_lights; ++light_index) {
        const PointLight& light = result.point_lights[light_index];
        LightBounds& bounds = light_bounds_[light_index];
        bounds = {};
        const Vec3 relative = subtract(light.position, camera.position);
        const float depth = dot(relative, forward);
        if (depth + light.range < camera.near_plane || depth - light.range > camera.far_plane)
            continue;
        const float minimum_depth = fmaxf(camera.near_plane, depth - light.range);
        const float maximum_depth = fminf(camera.far_plane, depth + light.range);
        bounds.minimum_z = static_cast<uint32>(logf(minimum_depth / camera.near_plane) / log_range * settings_.grid_depth);
        bounds.maximum_z = static_cast<uint32>(logf(maximum_depth / camera.near_plane) / log_range * settings_.grid_depth);
        if (bounds.minimum_z >= settings_.grid_depth)
            bounds.minimum_z = settings_.grid_depth - 1U;
        if (bounds.maximum_z >= settings_.grid_depth)
            bounds.maximum_z = settings_.grid_depth - 1U;
        const float safe_depth = fmaxf(depth, camera.near_plane);
        const float center_x = dot(relative, right) / (safe_depth * tangent_x);
        const float center_y = dot(relative, up) / (safe_depth * tangent_y);
        const float radius_x = light.range / (safe_depth * tangent_x);
        const float radius_y = light.range / (safe_depth * tangent_y);
        bounds.minimum_x = static_cast<uint32>(
            fminf(fmaxf(((center_x - radius_x) * 0.5F + 0.5F) * settings_.grid_width, 0.0F), static_cast<float>(settings_.grid_width - 1U)));
        bounds.maximum_x = static_cast<uint32>(
            fminf(fmaxf(((center_x + radius_x) * 0.5F + 0.5F) * settings_.grid_width, 0.0F), static_cast<float>(settings_.grid_width - 1U)));
        bounds.minimum_y = static_cast<uint32>(
            fminf(fmaxf(((-center_y - radius_y) * 0.5F + 0.5F) * settings_.grid_height, 0.0F), static_cast<float>(settings_.grid_height - 1U)));
        bounds.maximum_y = static_cast<uint32>(
            fminf(fmaxf(((-center_y + radius_y) * 0.5F + 0.5F) * settings_.grid_height, 0.0F), static_cast<float>(settings_.grid_height - 1U)));
        bounds.visible = true;
        for (uint32 z = bounds.minimum_z; z <= bounds.maximum_z; ++z)
            for (uint32 y = bounds.minimum_y; y <= bounds.maximum_y; ++y)
                for (uint32 x = bounds.minimum_x; x <= bounds.maximum_x; ++x) {
                    LightClusterRange& cluster = result.clusters[x + y * settings_.grid_width + z * settings_.grid_width * settings_.grid_height];
                    if (cluster.light_count < settings_.maximum_lights_per_cluster)
                        ++cluster.light_count;
                    else
                        ++result.metrics.saturated_clusters;
                }
    }
    uint32 first_light = 0;
    for (LightClusterRange& cluster : result.clusters) {
        cluster.first_light = first_light;
        first_light += cluster.light_count;
    }
    result.light_indices.reserve(first_light);
    result.light_indices.resize(first_light);
    // During fill, first_light is the cursor and light_count is the remaining capacity.
    for (uint32 light_index = 0; light_index < active_lights; ++light_index) {
        const LightBounds& bounds = light_bounds_[light_index];
        if (!bounds.visible)
            continue;
        for (uint32 z = bounds.minimum_z; z <= bounds.maximum_z; ++z)
            for (uint32 y = bounds.minimum_y; y <= bounds.maximum_y; ++y)
                for (uint32 x = bounds.minimum_x; x <= bounds.maximum_x; ++x) {
                    LightClusterRange& cluster = result.clusters[x + y * settings_.grid_width + z * settings_.grid_width * settings_.grid_height];
                    if (cluster.light_count) {
                        result.light_indices[cluster.first_light++] = light_index;
                        --cluster.light_count;
                    }
                }
    }
    first_light = 0;
    for (LightClusterRange& cluster : result.clusters) {
        assert(cluster.light_count == 0);
        cluster.light_count = cluster.first_light - first_light;
        cluster.first_light = first_light;
        first_light += cluster.light_count;
    }
    result.metrics.clusters = cluster_count;
    result.metrics.light_references = result.light_indices.size();
    result.metrics.build_nanoseconds = (performance_clock() - started) * 1000000000ULL / clock_frequency_;
}

} // namespace gloom::render
