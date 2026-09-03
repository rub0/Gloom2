#include <gloom/core/job_system.hpp>
#include <gloom/render/visibility.hpp>

#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

gloom::render::RenderInstance instance(const float x, const float z) {
    return {.mesh = {.value = 10},
            .material = {.value = 20},
            .transform = {.position = {x, 0.0F, z}},
            .local_bounds = {.radius = 1.0F},
            .lod_meshes = {{{.value = 10}, {.value = 11}, {.value = 12}}},
            .lod_count = 3};
}

} // namespace

int main() try {
    gloom::core::JobSystem jobs{{.worker_threads = 2}};
    jobs.start();
    gloom::render::VisibilitySystem visibility{
        jobs,
        {.instances_per_job = 2,
         .lod1_distance_in_radii = 30.0F,
         .lod2_distance_in_radii = 80.0F}};
    const gloom::render::Camera camera{.position = {0.0F, 0.0F, 0.0F},
                                       .target = {0.0F, 0.0F, 1.0F},
                                       .near_plane = 0.1F,
                                       .far_plane = 200.0F};
    const std::vector instances{
        instance(0.0F, 10.0F),
        instance(1.0F, 12.0F),
        instance(0.0F, 40.0F),
        instance(0.0F, 100.0F),
        instance(0.0F, -10.0F),
        instance(100.0F, 10.0F),
    };
    const auto result = visibility.build(camera, 16.0F / 9.0F, instances);
    require(result.metrics.submitted_instances == 6 &&
                result.metrics.visible_instances == 4 &&
                result.metrics.culled_instances == 2 &&
                result.metrics.culling_jobs == 3,
            "Parallel frustum culling metrics are incorrect");
    require(result.metrics.lod_instances == std::array<std::uint64_t, 3>{2, 1, 1},
            "Distance-based LOD selection is incorrect");
    require(result.batches.size() == 3 && result.batches.front().instance_count == 2,
            "Visible instances were not grouped into stable mesh/material batches");
    require(result.snapshot().instances.size() == 4 &&
                result.snapshot().batches.size() == 3,
            "Prepared visibility snapshot does not own its complete draw data");
    jobs.stop();
    std::cout << "Gloom visibility tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Visibility test failure: " << error.what() << '\n';
    return 1;
}
