#include <gloom/assets/asset_loader.hpp>
#include <gloom/assets/residency_coordinator.hpp>
#include <gloom/assets/scene_catalog.hpp>
#include <gloom/backends/diligent_renderer.hpp>
#include <gloom/backends/sdl_window.hpp>
#include <gloom/core/job_system.hpp>
#include <gloom/render/visibility.hpp>
#include <gloom/render/lighting.hpp>

#include <chrono>
#include <array>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

int main(const int argument_count, const char* const* arguments) try {
    if (argument_count != 5 && argument_count != 6) {
        std::cerr << "Usage: gloom_scene_viewer <source-root> <cache-root> "
                     "<game:/source.gltf> <cache:/output.gasset> [scene-copies]\n";
        return 2;
    }
    std::uint32_t scene_copies = 1;
    if (argument_count == 6) {
        const std::string_view text{arguments[5]};
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), scene_copies);
        if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() ||
            scene_copies == 0 || scene_copies > 100'000) {
            throw std::runtime_error{"Scene copies must be between 1 and 100000"};
        }
    }
    const auto source = gloom::assets::VirtualPath::parse(arguments[3]);
    const auto cooked = gloom::assets::VirtualPath::parse(arguments[4]);
    if (!source || !cooked || source->mount() != "game" || cooked->mount() != "cache") {
        throw std::runtime_error{"Viewer paths must use game:/ and cache:/ mounts"};
    }
    gloom::assets::VirtualFileSystem filesystem;
    filesystem.mount("game", std::filesystem::path{arguments[1]});
    filesystem.mount("cache", std::filesystem::path{arguments[2]});
    auto discovered = gloom::assets::discover_cooked_scene(filesystem, *source, *cooked);
    if (!discovered) {
        throw std::runtime_error{discovered.error()};
    }

    gloom::core::JobSystem jobs;
    gloom::backends::SdlWindow window{{.title = "Gloom cooked-scene viewer",
                                       .width = 1280,
                                       .height = 720,
                                       .resizable = true}};
    gloom::backends::DiligentRenderer renderer{window};
    jobs.start();
    window.start();
    renderer.start();
    {
        gloom::assets::AsyncAssetLoader loader{jobs, filesystem, discovered->catalog};
        gloom::assets::AssetResidencyCoordinator residency{
            jobs, loader, discovered->catalog, renderer};
        gloom::render::VisibilitySystem visibility{jobs};
        gloom::render::ClusteredLightingBuilder lighting_builder;
        const std::array point_lights{
            gloom::render::PointLight{.position = {-3.0F, 4.0F, -2.0F},
                                      .range = 10.0F,
                                      .color = {1.0F, 0.25F, 0.08F},
                                      .intensity = 20.0F},
            gloom::render::PointLight{.position = {3.0F, 2.5F, 2.0F},
                                      .range = 8.0F,
                                      .color = {0.08F, 0.35F, 1.0F},
                                      .intensity = 18.0F},
        };
        const auto ticket = residency.request_scene(
            discovered->scene, gloom::assets::AssetPriority::critical);
        auto drawable = window.drawable_size();
        std::vector<gloom::render::RenderInstance> stress_instances;
        std::uint64_t prepared_generation = 0;
        gloom::render::VisibilityMetrics last_visibility;
        while (window.poll_events()) {
            residency.update();
            const auto state = residency.state(ticket);
            if (state == gloom::assets::SceneResidencyState::failed) {
                throw std::runtime_error{std::string{residency.error(ticket)}};
            }
            const auto current = window.drawable_size();
            if (current != drawable) {
                drawable = current;
                renderer.resize(drawable.first, drawable.second);
            }
            renderer.begin_frame();
            const auto* scene = residency.scene(ticket);
            if (scene != nullptr && scene->generation != prepared_generation) {
                stress_instances.clear();
                stress_instances.reserve(scene->instances.size() * scene_copies);
                const auto side = static_cast<std::uint32_t>(
                    std::ceil(std::sqrt(static_cast<float>(scene_copies))));
                for (std::uint32_t copy = 0; copy < scene_copies; ++copy) {
                    const float offset_x =
                        (static_cast<float>(copy % side) - static_cast<float>(side) * 0.5F) * 4.0F;
                    const float offset_z =
                        (static_cast<float>(copy / side) - static_cast<float>(side) * 0.5F) * 4.0F;
                    for (auto instance : scene->instances) {
                        instance.transform.position.x += offset_x;
                        instance.transform.position.z += offset_z;
                        stress_instances.push_back(instance);
                    }
                }
                prepared_generation = scene->generation;
            }
            const std::span<const gloom::render::RenderInstance> instances =
                scene ? std::span<const gloom::render::RenderInstance>{stress_instances}
                      : std::span<const gloom::render::RenderInstance>{};
            const gloom::render::Camera camera{.position = {7.0F, 5.0F, -9.0F},
                                                .target = {0.0F, 1.0F, 0.0F}};
            const float aspect = drawable.second == 0
                                     ? 1.0F
                                     : static_cast<float>(drawable.first) /
                                           static_cast<float>(drawable.second);
            const auto visible = visibility.build(camera, aspect, instances);
            last_visibility = visible.metrics;
            const auto lighting = lighting_builder.build(camera, aspect, point_lights);
            const auto lighting_view = lighting.view();
            auto snapshot = visible.snapshot();
            snapshot.lighting = &lighting_view;
            renderer.draw(snapshot);
            renderer.end_frame();
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        std::cout << "Visibility: " << last_visibility.visible_instances << "/"
                  << last_visibility.submitted_instances << " visible, "
                  << last_visibility.batches << " batches, LODs "
                  << last_visibility.lod_instances[0] << "/"
                  << last_visibility.lod_instances[1] << "/"
                  << last_visibility.lod_instances[2] << ", "
                  << last_visibility.build_nanoseconds / 1'000 << " us CPU.\n";
    }
    renderer.stop();
    window.stop();
    jobs.stop();
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Scene viewer failed: " << error.what() << '\n';
    return 1;
}
