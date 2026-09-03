#include <gloom/assets/scene_gpu_bridge.hpp>

#include <array>
#include <iostream>
#include <stdexcept>

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

} // namespace

int main() try {
    gloom::assets::ImportedScene scene;
    scene.materials.push_back({
        .name = "copper",
        .base_color = {0.8F, 0.3F, 0.1F, 1.0F},
        .metallic = 0.9F,
        .roughness = 0.2F,
        .base_color_texture = 0,
        .metallic_roughness_texture = 0,
        .normal_texture = 0,
    });
    scene.textures.push_back({.image = 0});
    scene.images.push_back({.name = "packed"});
    scene.primitives.push_back({
        .vertices = {
            {.position = {0.0F, 1.0F, 0.0F}},
            {.position = {-1.0F, -1.0F, 0.0F}},
            {.position = {1.0F, -1.0F, 0.0F}},
        },
        .indices = {0, 1, 2},
        .material = 0,
    });

    constexpr std::array image_assets{gloom::render::RenderAssetId{0x9876}};
    const auto first =
        gloom::assets::build_gpu_scene_uploads(scene, {.value = 0x1234}, image_assets);
    const auto second =
        gloom::assets::build_gpu_scene_uploads(scene, {.value = 0x1234}, image_assets);
    require(first.meshes.size() == 1 && first.materials.size() == 1,
            "Scene did not produce mesh and material uploads");
    require(first.meshes[0].vertices.size() == 3 && first.meshes[0].indices.size() == 3,
            "Scene geometry was not preserved");
    require(first.primitives[0].mesh == first.meshes[0].id &&
                first.primitives[0].material == first.materials[0].id,
            "Primitive GPU bindings are inconsistent");
    require(first.meshes[0].id == second.meshes[0].id,
            "Derived GPU asset identifiers are not stable");
    require(first.materials[0].base_color_texture == image_assets[0] &&
                first.materials[0].metallic_roughness_texture == image_assets[0] &&
                first.materials[0].normal_texture == image_assets[0],
            "Scene texture bindings were not mapped into the GPU material");

    std::cout << "Gloom GPU-asset bridge tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "GPU-asset bridge test failure: " << error.what() << '\n';
    return 1;
}
