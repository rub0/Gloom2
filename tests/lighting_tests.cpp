#include <gloom/render/lighting.hpp>

#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

} // namespace

int main() try {
    gloom::render::ClusteredLightingBuilder builder{{.grid_width = 8,
                                                      .grid_height = 4,
                                                      .grid_depth = 8,
                                                      .maximum_lights = 3,
                                                      .maximum_lights_per_cluster = 2}};
    const gloom::render::Camera camera{.position = {0.0F, 0.0F, 0.0F},
                                       .target = {0.0F, 0.0F, 1.0F},
                                       .near_plane = 0.1F,
                                       .far_plane = 100.0F};
    const std::vector lights{
        gloom::render::PointLight{.position = {0.0F, 0.0F, 5.0F}, .range = 2.0F},
        gloom::render::PointLight{.position = {1.0F, 0.0F, 10.0F}, .range = 3.0F},
        gloom::render::PointLight{.position = {0.0F, 0.0F, -20.0F}, .range = 1.0F},
        gloom::render::PointLight{.position = {0.0F, 0.0F, 15.0F}, .range = -1.0F},
    };
    const auto result = builder.build(camera, 16.0F / 9.0F, lights);
    require(result.metrics.input_lights == 4 && result.metrics.active_lights == 3,
            "Cluster builder filtering or capacity metrics are incorrect");
    require(result.clusters.size() == 8U * 4U * 8U &&
                !result.light_indices.empty() && result.metrics.light_references > 0,
            "Cluster grid did not reference visible point lights");
    require(result.view().clusters.size() == result.clusters.size() &&
                result.view().point_lights.size() == 3,
            "Prepared lighting view lost owned cluster data");
    for (const auto& cluster : result.clusters) {
        require(cluster.light_count <= 2 &&
                    cluster.first_light + cluster.light_count <= result.light_indices.size(),
                "Cluster range exceeds its bounded light list");
    }
    require(!gloom::render::ClusteredLightingBuilder{}
                 .build(camera, 1.0F, std::span<const gloom::render::PointLight>{})
                 .clusters.empty(),
            "Empty light set did not preserve the complete cluster grid");
    std::cout << "Gloom clustered-lighting tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Lighting test failure: " << error.what() << '\n';
    return 1;
}
