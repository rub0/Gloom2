#pragma once

#include <gloom/core/job_system.hpp>
#include <gloom/core/array.hpp>
#include <gloom/core/clock.hpp>
#include <gloom/render/scene.hpp>

namespace gloom::render {

struct VisibilitySettings {
    uint32 instances_per_job{128};
    float lod1_distance_in_radii{30.0F};
    float lod2_distance_in_radii{80.0F};
};

struct VisibilityMetrics {
    uint64 submitted_instances{0};
    uint64 visible_instances{0};
    uint64 culled_instances{0};
    uint64 lod_instances[3]{};
    uint64 batches{0};
    uint64 culling_jobs{0};
    uint64 build_nanoseconds{0};
};

struct PreparedVisibility {
    Camera camera;
    Array<RenderInstance> instances;
    Array<RenderBatch> batches;
    VisibilityMetrics metrics;

    [[nodiscard]] RenderSnapshot snapshot() const noexcept;
};

class VisibilitySystem final {
  public:
    explicit VisibilitySystem(core::JobSystem& jobs, VisibilitySettings settings = {});

    // Caller owns the result; snapshots remain valid until it is rebuilt/destroyed.
    // Do not alias input with result.instances or build concurrently on this system.
    void build(PreparedVisibility& result, const Camera& camera, float aspect_ratio, Span<const RenderInstance> instances);

  private:
    core::JobSystem& jobs_;
    VisibilitySettings settings_;
    struct VisibleIndex {
        size_t source{0};
        RenderAssetId mesh;
        RenderAssetId material;
    };
    struct JobOutput {
        size_t count{0};
        uint64 lod_instances[3]{};
    };
    Array<VisibleIndex> visible_indices_;
    Array<JobOutput> job_outputs_;
    uint64 clock_frequency_{performance_frequency()};
    static int compare_indices(const void* left, const void* right);
};

} // namespace gloom::render
