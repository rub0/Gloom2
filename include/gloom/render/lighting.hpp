#pragma once

#include <gloom/render/scene.hpp>
#include <gloom/core/array.hpp>
#include <gloom/core/clock.hpp>

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
    uint32 size{0};
    uint32 mip_levels{0};
    // Face-major (+X,-X,+Y,-Y,+Z,-Z), then mip-major linear HDR RGBA.
    std::vector<std::array<float, 4>> radiance;
};

struct EnvironmentLighting {
    Vec3 sky_radiance{0.12F, 0.18F, 0.30F};
    Vec3 ground_radiance{0.025F, 0.02F, 0.018F};
    float intensity{1.0F};
    float exposure{1.0F};
    // Borrowed: owner outlives all prepared views and renderer uses; reset to null before destroying it.
    const EnvironmentProbe* probe{nullptr};
    float ambient_fill{0.0F};
};

struct LightClusterRange {
    uint32 first_light{0};
    uint32 light_count{0};
};

struct LightClusterGrid {
    uint32 width{16};
    uint32 height{9};
    uint32 depth{24};
    float near_plane{0.1F};
    float far_plane{500.0F};
};

struct ClusteredLightingView {
    DirectionalLight directional;
    EnvironmentLighting environment;
    LightClusterGrid grid;
    Span<const PointLight> point_lights;
    Span<const LightClusterRange> clusters;
    Span<const uint32> light_indices;
};

struct LightingMetrics {
    uint64 input_lights{0};
    uint64 active_lights{0};
    uint64 clusters{0};
    uint64 light_references{0};
    uint64 saturated_clusters{0};
    uint64 build_nanoseconds{0};
};

struct PreparedLighting {
    DirectionalLight directional;
    EnvironmentLighting environment;
    LightClusterGrid grid;
    Array<PointLight> point_lights;
    Array<LightClusterRange> clusters;
    Array<uint32> light_indices;
    LightingMetrics metrics;

    [[nodiscard]] ClusteredLightingView view() const noexcept;
};

struct LightingSettings {
    uint32 grid_width{16};
    uint32 grid_height{9};
    uint32 grid_depth{24};
    uint32 maximum_lights{256};
    uint32 maximum_lights_per_cluster{64};
};

class ClusteredLightingBuilder final {
  public:
    explicit ClusteredLightingBuilder(LightingSettings settings = {});

    // Caller owns the result; views remain valid until it is rebuilt/destroyed.
    // Input lights must not alias result.point_lights.
    // Do not build concurrently on this builder.
    void build(PreparedLighting& result, const Camera& camera, float aspect_ratio, Span<const PointLight> lights, const DirectionalLight& directional = {},
        const EnvironmentLighting& environment = {});

  private:
    LightingSettings settings_;
    struct LightBounds {
        uint32 minimum_x{0}, maximum_x{0};
        uint32 minimum_y{0}, maximum_y{0};
        uint32 minimum_z{0}, maximum_z{0};
        bool visible{false};
    };
    Array<LightBounds> light_bounds_;
    uint64 clock_frequency_{performance_frequency()};
};

} // namespace gloom::render
