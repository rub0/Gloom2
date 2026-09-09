#include <gloom/render/lighting.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace gloom::render {
namespace {

[[nodiscard]] Vec3 subtract(const Vec3 left, const Vec3 right) noexcept {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}
[[nodiscard]] float dot(const Vec3 left, const Vec3 right) noexcept {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}
[[nodiscard]] Vec3 cross(const Vec3 left, const Vec3 right) noexcept {
    return {left.y * right.z - left.z * right.y,
            left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}
[[nodiscard]] Vec3 normalize(const Vec3 value) noexcept {
    const float magnitude = std::sqrt(dot(value, value));
    return magnitude > 1.0e-7F
               ? Vec3{value.x / magnitude, value.y / magnitude, value.z / magnitude}
               : Vec3{};
}

} // namespace

ClusteredLightingView PreparedLighting::view() const noexcept {
    return {.directional = directional,
            .environment = environment,
            .grid = grid,
            .point_lights = point_lights,
            .clusters = clusters,
            .light_indices = light_indices};
}

ClusteredLightingBuilder::ClusteredLightingBuilder(const LightingSettings settings)
    : settings_{settings} {
    if (settings_.grid_width == 0 || settings_.grid_height == 0 ||
        settings_.grid_depth == 0 || settings_.maximum_lights == 0 ||
        settings_.maximum_lights_per_cluster == 0) {
        throw std::invalid_argument{"Clustered-lighting settings must be non-zero"};
    }
}

PreparedLighting ClusteredLightingBuilder::build(const Camera& camera,
                                                  const float aspect_ratio,
                                                  const std::span<const PointLight> lights,
                                                  const DirectionalLight directional,
                                                  const EnvironmentLighting environment) const {
    const auto started = std::chrono::steady_clock::now();
    if (!(aspect_ratio > 0.0F) || camera.near_plane <= 0.0F ||
        camera.far_plane <= camera.near_plane) {
        throw std::invalid_argument{"Camera is invalid for clustered lighting"};
    }
    PreparedLighting result{.directional = directional,
                            .environment = environment,
                            .grid = {.width = settings_.grid_width,
                                     .height = settings_.grid_height,
                                     .depth = settings_.grid_depth,
                                     .near_plane = camera.near_plane,
                                     .far_plane = camera.far_plane}};
    const std::size_t cluster_count = static_cast<std::size_t>(settings_.grid_width) *
                                      settings_.grid_height * settings_.grid_depth;
    result.point_lights.reserve(settings_.maximum_lights);
    result.clusters.reserve(cluster_count);
    result.light_indices.reserve(cluster_count * 4U);
    result.metrics.input_lights = lights.size();
    for (const auto& light : lights) {
        if (result.point_lights.size() >= settings_.maximum_lights) {
            break;
        }
        if (light.range > 0.0F && light.intensity > 0.0F &&
            std::isfinite(light.range) && std::isfinite(light.intensity)) {
            result.point_lights.push_back(light);
        }
    }
    result.metrics.active_lights = result.point_lights.size();

    const Vec3 forward = normalize(subtract(camera.target, camera.position));
    const Vec3 right = normalize(cross(camera.up, forward));
    const Vec3 up = cross(forward, right);
    const float tangent_y = std::tan(camera.vertical_field_of_view_radians * 0.5F);
    const float tangent_x = tangent_y * aspect_ratio;
    const float log_range = std::log(camera.far_plane / camera.near_plane);
    std::vector<std::uint32_t> assignment_counts(cluster_count, 0);

    for (std::uint32_t light_index = 0; light_index < result.point_lights.size(); ++light_index) {
        const auto& light = result.point_lights[light_index];
        const Vec3 relative = subtract(light.position, camera.position);
        const float depth = dot(relative, forward);
        if (depth + light.range < camera.near_plane ||
            depth - light.range > camera.far_plane) {
            continue;
        }
        const float minimum_depth = std::max(camera.near_plane, depth - light.range);
        const float maximum_depth = std::min(camera.far_plane, depth + light.range);
        const auto depth_slice = [&](const float value) {
            const float normalized = std::log(value / camera.near_plane) / log_range;
            return std::min(static_cast<std::uint32_t>(normalized * settings_.grid_depth),
                            settings_.grid_depth - 1U);
        };
        const std::uint32_t minimum_z = depth_slice(minimum_depth);
        const std::uint32_t maximum_z = depth_slice(maximum_depth);
        const float safe_depth = std::max(depth, camera.near_plane);
        const float center_x = dot(relative, right) / (safe_depth * tangent_x);
        const float center_y = dot(relative, up) / (safe_depth * tangent_y);
        const float radius_x = light.range / (safe_depth * tangent_x);
        const float radius_y = light.range / (safe_depth * tangent_y);
        const auto tile_min = [](const float ndc, const std::uint32_t count) {
            return static_cast<std::uint32_t>(std::clamp(
                (ndc * 0.5F + 0.5F) * count, 0.0F, static_cast<float>(count - 1U)));
        };
        const std::uint32_t minimum_x = tile_min(center_x - radius_x, settings_.grid_width);
        const std::uint32_t maximum_x = tile_min(center_x + radius_x, settings_.grid_width);
        const std::uint32_t minimum_y = tile_min(-center_y - radius_y, settings_.grid_height);
        const std::uint32_t maximum_y = tile_min(-center_y + radius_y, settings_.grid_height);
        for (std::uint32_t z = minimum_z; z <= maximum_z; ++z) {
            for (std::uint32_t y = minimum_y; y <= maximum_y; ++y) {
                for (std::uint32_t x = minimum_x; x <= maximum_x; ++x) {
                    auto& cluster_count_for_cell = assignment_counts[x + y * settings_.grid_width +
                                                                       z * settings_.grid_width * settings_.grid_height];
                    if (cluster_count_for_cell < settings_.maximum_lights_per_cluster) {
                        ++cluster_count_for_cell;
                    } else {
                        ++result.metrics.saturated_clusters;
                    }
                }
            }
        }
    }

    std::uint32_t first_light = 0;
    for (const auto count : assignment_counts) {
        result.clusters.push_back({
            .first_light = first_light,
            .light_count = count,
        });
        first_light += count;
    }
    result.light_indices.resize(first_light);
    std::vector<std::uint32_t> assignment_offsets(cluster_count);
    for (std::size_t index = 0; index < cluster_count; ++index) {
        assignment_offsets[index] = result.clusters[index].first_light;
        assignment_counts[index] = 0;
    }

    for (std::uint32_t light_index = 0; light_index < result.point_lights.size(); ++light_index) {
        const auto& light = result.point_lights[light_index];
        const Vec3 relative = subtract(light.position, camera.position);
        const float depth = dot(relative, forward);
        if (depth + light.range < camera.near_plane || depth - light.range > camera.far_plane) continue;
        const float minimum_depth = std::max(camera.near_plane, depth - light.range);
        const float maximum_depth = std::min(camera.far_plane, depth + light.range);
        const auto depth_slice = [&](const float value) {
            const float normalized = std::log(value / camera.near_plane) / log_range;
            return std::min(static_cast<std::uint32_t>(normalized * settings_.grid_depth), settings_.grid_depth - 1U);
        };
        const std::uint32_t minimum_z = depth_slice(minimum_depth);
        const std::uint32_t maximum_z = depth_slice(maximum_depth);
        const float safe_depth = std::max(depth, camera.near_plane);
        const float center_x = dot(relative, right) / (safe_depth * tangent_x);
        const float center_y = dot(relative, up) / (safe_depth * tangent_y);
        const float radius_x = light.range / (safe_depth * tangent_x);
        const float radius_y = light.range / (safe_depth * tangent_y);
        const auto tile_min = [](const float ndc, const std::uint32_t count) {
            return static_cast<std::uint32_t>(std::clamp((ndc * 0.5F + 0.5F) * count, 0.0F, static_cast<float>(count - 1U)));
        };
        const std::uint32_t minimum_x = tile_min(center_x - radius_x, settings_.grid_width);
        const std::uint32_t maximum_x = tile_min(center_x + radius_x, settings_.grid_width);
        const std::uint32_t minimum_y = tile_min(-center_y - radius_y, settings_.grid_height);
        const std::uint32_t maximum_y = tile_min(-center_y + radius_y, settings_.grid_height);
        for (std::uint32_t z = minimum_z; z <= maximum_z; ++z) {
            for (std::uint32_t y = minimum_y; y <= maximum_y; ++y) {
                for (std::uint32_t x = minimum_x; x <= maximum_x; ++x) {
                    const std::size_t cluster_index = x + y * settings_.grid_width + z * settings_.grid_width * settings_.grid_height;
                    const std::uint32_t offset = assignment_offsets[cluster_index];
                    const std::uint32_t used = assignment_counts[cluster_index];
                    if (used < result.clusters[cluster_index].light_count) {
                        result.light_indices[offset + used] = light_index;
                        ++assignment_counts[cluster_index];
                    }
                }
            }
        }
    }
    result.metrics.clusters = result.clusters.size();
    result.metrics.light_references = result.light_indices.size();
    result.metrics.build_nanoseconds = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - started)
            .count());
    return result;
}

} // namespace gloom::render
