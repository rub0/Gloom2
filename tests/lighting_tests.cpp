#include <gloom/render/lighting.hpp>

#include <stdio.h>
#include <stdlib.h>

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

} // namespace

int main() {
    gloom::render::ClusteredLightingBuilder builder{{.grid_width = 8, .grid_height = 4, .grid_depth = 8, .maximum_lights = 3, .maximum_lights_per_cluster = 2}};
    const gloom::render::Camera camera{.position = {0.0F, 0.0F, 0.0F}, .target = {0.0F, 0.0F, 1.0F}, .near_plane = 0.1F, .far_plane = 100.0F};
    const gloom::render::PointLight lights[]{
        gloom::render::PointLight{.position = {0.0F, 0.0F, 5.0F}, .range = 2.0F},
        gloom::render::PointLight{.position = {1.0F, 0.0F, 10.0F}, .range = 3.0F},
        gloom::render::PointLight{.position = {0.0F, 0.0F, -20.0F}, .range = 1.0F},
        gloom::render::PointLight{.position = {0.0F, 0.0F, 15.0F}, .range = -1.0F},
    };
    gloom::render::PreparedLighting result;
    builder.build(result, camera, 16.0F / 9.0F, lights);
    require(result.metrics.input_lights == 4 && result.metrics.active_lights == 3, "Cluster builder filtering or capacity metrics are incorrect");
    require(result.clusters.size() == 8U * 4U * 8U && result.light_indices.size() > 0 && result.metrics.light_references > 0,
        "Cluster grid did not reference visible point lights");
    require(
        result.view().clusters.size() == result.clusters.size() && result.view().point_lights.size() == 3, "Prepared lighting view lost owned cluster data");
    for (const gloom::render::LightClusterRange& cluster : result.clusters) {
        require(cluster.light_count <= 2 && cluster.first_light + cluster.light_count <= result.light_indices.size(),
            "Cluster range exceeds its bounded light list");
    }
    gloom::render::PreparedLighting empty;
    gloom::render::ClusteredLightingBuilder{}.build(empty, camera, 1.0F, {});
    require(empty.clusters.size() > 0, "Empty light set did not preserve the complete cluster grid");
    const gloom::render::PointLight* light_storage = result.point_lights.data();
    const gloom::render::LightClusterRange* cluster_storage = result.clusters.data();
    const gloom::uint32* index_storage = result.light_indices.data();
    const gloom::uint64 references = result.metrics.light_references;
    for (gloom::uint32 repeat = 0; repeat < 32; ++repeat) {
        builder.build(result, camera, 16.0F / 9.0F, lights);
        require(result.point_lights.data() == light_storage && result.clusters.data() == cluster_storage && result.light_indices.data() == index_storage,
            "Repeated lighting frame grew output storage");
        require(result.metrics.light_references == references, "Repeated lighting frame retained old counts");
    }
    builder.build(empty, camera, 1.0F, {});
    require(empty.clusters.size() == 256 && empty.light_indices.size() == 0 && result.metrics.light_references == references,
        "Empty/independent lighting result invalidated owned data");
    // Every light covers every cell. Keep the first two; count saturation exactly.
    const gloom::render::PointLight covering[4]{{.range = 1000}, {.range = 1000}, {.range = 1000}, {.range = -1}};
    builder.build(result, camera, 1.0F, covering);
    require(result.light_indices.size() == 512 && result.metrics.saturated_clusters == 256 && result.metrics.active_lights == 3,
        "Saturated grid counts are incorrect");
    for (gloom::uint32 index = 0; index < 256; ++index) {
        require(result.clusters[index].first_light == index * 2 && result.clusters[index].light_count == 2, "Saturated prefix ranges are incorrect");
        require(result.light_indices[index * 2] == 0 && result.light_indices[index * 2 + 1] == 1, "Saturation changed light priority/order");
    }
    builder.build(result, camera, 1.0F, {});
    require(result.point_lights.size() == 0 && result.light_indices.size() == 0 && result.metrics.saturated_clusters == 0, "Empty frame retained old lights");
    for (const gloom::render::LightClusterRange& cluster : result.clusters)
        require(cluster.first_light == 0 && cluster.light_count == 0, "Empty frame retained old cluster ranges");
    builder.build(result, camera, 16.0F / 9.0F, lights);
    require(result.metrics.light_references == references, "Lighting failed to recover after empty frame");
    puts("Gloom clustered-lighting tests completed successfully.");
    return 0;
}
