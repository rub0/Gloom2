#pragma once
#include <gloom/network/movement_replication.hpp>
#include <gloom/gameplay/slice_selection.hpp>
#include <gloom/gameplay/legacy_pickups.hpp>
#include <gloom/physics/world.hpp>
#include <gloom/render/lighting.hpp>
#include <filesystem>
#include <expected>
#include <string>
#include <vector>

namespace gloom::gameplay {
struct FactorySpawn {
    physics::Vec3 position;
    float yaw{0.0F};
};
struct FactoryJumper {
    physics::Vec3 position, force, half_extent;
};
struct FactoryScene {
    uint32 scene_id{0};
    render::EnvironmentProbe environment_probe;
    physics::TriangleMesh collision;
    std::vector<FactorySpawn> spawns;
    std::vector<PickupDefinition> pickups;
    std::vector<render::PointLight> lights;
    FactoryJumper jumper;
    physics::Vec3 lava_center;
    float lava_half_width{112.5F};
    size_t entity_count{0};
};
[[nodiscard]] const FactoryScene& original_factory();
// Owned external data; no views point into either JSON parser. root contains the three Factory files.
[[nodiscard]] std::expected<FactoryScene, std::string> load_factory_scene(const std::filesystem::path& root);
// Initializes the immutable process-lifetime scene; check during application initialization before original_factory().
[[nodiscard]] const char* original_factory_error();
[[nodiscard]] bool inside_factory_jumper(physics::Vec3 position) noexcept;
struct FactoryCharacterResolver {
    SliceCharacter (*function)(void*, network::NetworkEntityId){nullptr};
    void* context{nullptr};
};
// character and its context are borrowed and must outlive every copy of the returned settings.
[[nodiscard]] network::ReplicationSettings factory_movement_settings(FactoryCharacterResolver* character = nullptr);
}
