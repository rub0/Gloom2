#pragma once

#include <gloom/core/job_system.hpp>
#include <gloom/render/scene.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace gloom::render {

struct VisibilitySettings {
    std::size_t instances_per_job{128};
    float lod1_distance_in_radii{30.0F};
    float lod2_distance_in_radii{80.0F};
};

struct VisibilityMetrics {
    std::uint64_t submitted_instances{0};
    std::uint64_t visible_instances{0};
    std::uint64_t culled_instances{0};
    std::array<std::uint64_t, 3> lod_instances{};
    std::uint64_t batches{0};
    std::uint64_t culling_jobs{0};
    std::uint64_t build_nanoseconds{0};
};

struct PreparedVisibility {
    Camera camera;
    std::vector<RenderInstance> instances;
    std::vector<RenderBatch> batches;
    VisibilityMetrics metrics;

    [[nodiscard]] RenderSnapshot snapshot() const noexcept;
};

class VisibilitySystem final {
public:
    explicit VisibilitySystem(core::JobSystem& jobs, VisibilitySettings settings = {});

    [[nodiscard]] PreparedVisibility build(const Camera& camera,
                                           float aspect_ratio,
                                           std::span<const RenderInstance> instances);

private:
    core::JobSystem& jobs_;
    VisibilitySettings settings_;
};

} // namespace gloom::render
