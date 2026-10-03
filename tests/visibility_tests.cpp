#include <gloom/core/job_system.hpp>
#include <gloom/render/visibility.hpp>

#include <stdio.h>
#include <stdlib.h>

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

gloom::render::RenderInstance instance(const float x, const float z) {
    return {.mesh = {.value = 10},
        .material = {.value = 20},
        .transform = {.position = {.x = x, .z = z}},
        .local_bounds = {.radius = 1.0F},
        .lod_meshes = {{{.value = 10}, {.value = 11}, {.value = 12}}},
        .lod_count = 3};
}

} // namespace

int main() {
    gloom::core::JobSystem jobs{{.worker_threads = 2}};
    jobs.start();
    gloom::render::VisibilitySystem visibility{jobs, {.instances_per_job = 2, .lod1_distance_in_radii = 30.0F, .lod2_distance_in_radii = 80.0F}};
    const gloom::render::Camera camera{.position = {0.0F, 0.0F, 0.0F}, .target = {0.0F, 0.0F, 1.0F}, .near_plane = 0.1F, .far_plane = 200.0F};
    const gloom::render::RenderInstance instances[]{
        instance(0.0F, 10.0F),
        instance(1.0F, 12.0F),
        instance(0.0F, 40.0F),
        instance(0.0F, 100.0F),
        instance(0.0F, -10.0F),
        instance(100.0F, 10.0F),
    };
    gloom::render::PreparedVisibility result;
    visibility.build(result, camera, 16.0F / 9.0F, instances);
    require(result.metrics.submitted_instances == 6 && result.metrics.visible_instances == 4 && result.metrics.culled_instances == 2 &&
                result.metrics.culling_jobs == 3,
        "Parallel frustum culling metrics are incorrect");
    require(result.metrics.lod_instances[0] == 2 && result.metrics.lod_instances[1] == 1 && result.metrics.lod_instances[2] == 1,
        "Distance-based LOD selection is incorrect");
    require(result.batches.size() == 3 && result.batches[0].instance_count == 2, "Visible instances were not grouped into stable mesh/material batches");
    require(
        result.snapshot().instances.size() == 4 && result.snapshot().batches.size() == 3, "Prepared visibility snapshot does not own its complete draw data");
    const gloom::render::RenderInstance* storage = result.instances.data();
    const gloom::render::RenderBatch* batch_storage = result.batches.data();
    for (gloom::uint32 repeat = 0; repeat < 32; ++repeat) {
        visibility.build(result, camera, 16.0F / 9.0F, instances);
        require(result.instances.data() == storage && result.batches.data() == batch_storage, "Repeated frame grew output storage");
        require(result.metrics.visible_instances == 4 && result.batches[0].instance_count == 2, "Repeated frame retained old counters");
    }
    gloom::render::PreparedVisibility independent;
    visibility.build(independent, camera, 1.0F, {});
    require(result.instances.size() == 4 && independent.instances.size() == 0 && independent.metrics.culling_jobs == 0,
        "Building another result invalidated owned data");
    visibility.build(result, camera, 1.0F, {instances + 4, 2});
    require(result.instances.size() == 0 && result.batches.size() == 0 && result.metrics.culled_instances == 2, "Culled frame retained old output");
    // Cross several jobs, exercise reflected/non-uniform bounds and equal-key order.
    gloom::render::RenderInstance many[1025];
    for (gloom::uint32 index = 0; index < 1025; ++index) {
        many[index] = instance(0, 10);
        many[index].material.value = 20 + index % 7;
        many[index].source_node = index;
        many[index].transform.scale = {.x = -2, .y = 3, .z = .5F};
    }
    visibility.build(result, camera, 1.0F, many);
    require(result.instances.size() == 1025 && result.batches.size() == 7, "Large frame lost instances or batches");
    for (size_t index = 1; index < result.instances.size(); ++index)
        require(result.instances[index - 1].material.value < result.instances[index].material.value ||
                    (result.instances[index - 1].material == result.instances[index].material &&
                        result.instances[index - 1].source_node < result.instances[index].source_node),
            "Visible indices lost deterministic grouping");
    visibility.build(result, camera, 1.0F, instances);
    require(result.instances.size() == 4 && result.batches.size() == 3, "Shrinking after growth retained stale output");
    jobs.stop();
    puts("Gloom visibility tests completed successfully.");
    return 0;
}
