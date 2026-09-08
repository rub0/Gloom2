#pragma once
#include <gloom/network/movement_replication.hpp>
#include <gloom/gameplay/slice_selection.hpp>
#include <gloom/gameplay/legacy_pickups.hpp>
#include <gloom/physics/world.hpp>
#include <gloom/render/lighting.hpp>
#include <memory>
#include <string>
#include <vector>

namespace gloom::gameplay {
struct FactorySpawn { physics::Vec3 position; float yaw{0.0F}; };
struct FactoryJumper { physics::Vec3 position,force,half_extent; };
struct FactoryScene {
    std::uint32_t scene_id{0};
    std::shared_ptr<const render::EnvironmentProbe> environment_probe;
    std::shared_ptr<const physics::TriangleMesh> collision;
    std::vector<FactorySpawn> spawns;
    std::vector<PickupDefinition> pickups;
    std::vector<render::PointLight> lights;
    FactoryJumper jumper;
    physics::Vec3 lava_center;
    float lava_half_width{112.5F};
    std::size_t entity_count{0};
};
[[nodiscard]] const FactoryScene& original_factory();
[[nodiscard]] bool inside_factory_jumper(physics::Vec3 position) noexcept;
[[nodiscard]] network::ReplicationSettings factory_movement_settings(
    std::function<SliceCharacter(network::NetworkEntityId)> character = {});
}
