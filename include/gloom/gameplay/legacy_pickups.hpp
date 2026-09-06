#pragma once
#include <gloom/gameplay/legacy_arsenal.hpp>
#include <gloom/physics/world.hpp>
#include <span>
#include <string>
#include <vector>

namespace gloom::gameplay {
inline constexpr std::size_t factory_pickup_count = 73;
enum class PickupKind : std::uint8_t { life, shield, weapon, ammo, damage, cooldown };
enum class PickupPhase : std::uint8_t { available, pulling, respawning };
struct PickupDefinition {
    std::string name;
    PickupKind kind{};
    SliceWeapon weapon{};
    physics::Vec3 position;
    std::uint16_t reward{};
    std::uint16_t respawn_ticks{};
    float radius{.45F};
    std::uint32_t source_node{~0U};
};
struct PickupView {
    physics::Vec3 position;
    std::uint16_t respawn_remaining{};
    PickupPhase phase{PickupPhase::available};
    std::uint8_t pulling_player{};
};
struct PickupActor {
    std::uint8_t id{};
    physics::Vec3 feet;
    bool alive{};
    bool hold_pull{};
    LegacyArsenal* arsenal{};
    float* life{};
    float* shield{};
};
// Array order is the stable map identity. Only the authority mutates this state.
class LegacyPickups {
public:
    void reset(std::span<const PickupDefinition> definitions);
    void tick(std::span<PickupActor> actors);
    bool pull(const PickupActor& actor, physics::Vec3 direction, float range);
    [[nodiscard]] std::span<const PickupView> views() const { return states_; }
private:
    std::vector<PickupDefinition> definitions_;
    std::vector<PickupView> states_;
};
}
