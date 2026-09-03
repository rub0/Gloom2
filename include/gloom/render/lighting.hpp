#pragma once

#include <gloom/render/scene.hpp>

#include <cstdint>
#include <span>
#include <vector>
#include <memory>

namespace gloom::render {

struct DirectionalLight {
    Vec3 direction{-0.4F, -0.8F, 0.3F};
    Vec3 color{1.0F, 0.96F, 0.88F};
    float intensity{3.0F};
    bool casts_shadows{true};
};

struct PointLight {
    Vec3 position;
    float range{5.0F};
    Vec3 color{1.0F, 1.0F, 1.0F};
    float intensity{10.0F};
};

struct EnvironmentProbe {
    std::uint32_t size{0};
    std::uint32_t mip_levels{0};
    // Face-major (+X,-X,+Y,-Y,+Z,-Z), then mip-major linear HDR RGBA.
    std::vector<std::array<float, 4>> radiance;
};

struct EnvironmentLighting {
    Vec3 sky_radiance{0.12F, 0.18F, 0.30F};
    Vec3 ground_radiance{0.025F, 0.02F, 0.018F};
    float intensity{1.0F};
    float exposure{1.0F};
    std::shared_ptr<const EnvironmentProbe> probe;
};

struct LightClusterRange {
    std::uint32_t first_light{0};
    std::uint32_t light_count{0};
};

struct LightClusterGrid {
    std::uint32_t width{16};
    std::uint32_t height{9};
    std::uint32_t depth{24};
    float near_plane{0.1F};
    float far_plane{500.0F};
};

struct ClusteredLightingView {
    DirectionalLight directional;
    EnvironmentLighting environment;
    LightClusterGrid grid;
    std::span<const PointLight> point_lights;
    std::span<const LightClusterRange> clusters;
    std::span<const std::uint32_t> light_indices;
};

struct LightingMetrics {
    std::uint64_t input_lights{0};
    std::uint64_t active_lights{0};
    std::uint64_t clusters{0};
    std::uint64_t light_references{0};
    std::uint64_t saturated_clusters{0};
    std::uint64_t build_nanoseconds{0};
};

struct PreparedLighting {
    DirectionalLight directional;
    EnvironmentLighting environment;
    LightClusterGrid grid;
    std::vector<PointLight> point_lights;
    std::vector<LightClusterRange> clusters;
    std::vector<std::uint32_t> light_indices;
    LightingMetrics metrics;

    [[nodiscard]] ClusteredLightingView view() const noexcept;
};

struct LightingSettings {
    std::uint32_t grid_width{16};
    std::uint32_t grid_height{9};
    std::uint32_t grid_depth{24};
    std::uint32_t maximum_lights{256};
    std::uint32_t maximum_lights_per_cluster{64};
};

class ClusteredLightingBuilder final {
public:
    explicit ClusteredLightingBuilder(LightingSettings settings = {});

    [[nodiscard]] PreparedLighting build(const Camera& camera,
                                         float aspect_ratio,
                                         std::span<const PointLight> lights,
                                         DirectionalLight directional = {},
                                         EnvironmentLighting environment = {}) const;

private:
    LightingSettings settings_;
};

} // namespace gloom::render
