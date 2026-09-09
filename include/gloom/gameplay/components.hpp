#pragma once

#include <gloom/core/entity.hpp>
#include <gloom/gameplay/slice_selection.hpp>
#include <gloom/gameplay/legacy_arsenal.hpp>
#include <gloom/network/movement_replication.hpp>
#include <gloom/network/transport.hpp>
#include <gloom/physics/world.hpp>

#include <cstdint>
#include <array>
#include <utility>

namespace gloom::gameplay {

// Gameplay policy attached to a logical entity. Collision detection belongs to
// a physics TriggerComponent; this component only describes the consequence.
struct DamageVolumeComponent {
    float damage_per_second{60.0F};
};

struct TransformComponent {
    float position_x{0.0F};
    float position_y{0.0F};
    float position_z{0.0F};
    float velocity_x{0.0F};
    float velocity_y{0.0F};
    float velocity_z{0.0F};
};

// Stable logical contract for the migrated legacy controller. The backend
// handle is optional so data-only/client compositions remain backend neutral.
class CharacterPhysicsComponent final {
public:
    float radius{0.4F};
    float collision_height{1.8F};
    float maximum_slope_radians{0.872664626F};
    float step_up_height{0.4F};
    float step_down_height{0.5F};
    float mass{70.0F};
    float maximum_push_force{100.0F};
    bool provisional_kinematic_proxy{false};

    CharacterPhysicsComponent() = default;
    ~CharacterPhysicsComponent() { release(); }
    CharacterPhysicsComponent(const CharacterPhysicsComponent&) = delete;
    CharacterPhysicsComponent& operator=(const CharacterPhysicsComponent&) = delete;
    CharacterPhysicsComponent(CharacterPhysicsComponent&& other) noexcept { move_from(other); }
    CharacterPhysicsComponent& operator=(CharacterPhysicsComponent&& other) noexcept {
        if (this != &other) { release(); move_from(other); }
        return *this;
    }

    void attach(physics::World& world, core::EntityId entity, physics::Vec3 position) {
        reset();
        physics::CharacterDesc description{
            .position = position,
            .radius = radius,
            .cylinder_half_height = collision_height * 0.5F - radius,
            .max_slope_angle_radians = maximum_slope_radians,
            .step_up_height = step_up_height,
            .step_down_height = step_down_height,
            .mass = mass,
            .maximum_push_force = maximum_push_force,
            .owner = entity,
        };
        character_ = world.create_character(description);
        world_ = &world;
        world_generation_ = world.instance_generation();
    }

    void reset() {
        if (world_ != nullptr && character_.valid() &&
            world_->state() == core::SubsystemState::running &&
            world_->instance_generation() == world_generation_) {
            world_->destroy_character(character_);
        }
        world_ = nullptr; character_ = {}; world_generation_ = 0;
    }
    [[nodiscard]] bool attached() const noexcept { return character_.valid(); }
    [[nodiscard]] physics::CharacterId character() const noexcept { return character_; }

private:
    void release() noexcept { try { reset(); } catch (...) { world_ = nullptr; character_ = {}; } }
    void move_from(CharacterPhysicsComponent& other) noexcept {
        radius = other.radius; collision_height = other.collision_height;
        maximum_slope_radians = other.maximum_slope_radians;
        step_up_height = other.step_up_height; step_down_height = other.step_down_height;
        mass = other.mass; maximum_push_force = other.maximum_push_force;
        provisional_kinematic_proxy = other.provisional_kinematic_proxy;
        world_ = std::exchange(other.world_, nullptr);
        character_ = std::exchange(other.character_, {});
        world_generation_ = std::exchange(other.world_generation_, 0);
    }
    physics::World* world_{nullptr};
    physics::CharacterId character_;
    std::uint64_t world_generation_{0};
};

struct HealthComponent {
    float life{120.0F};
    std::uint32_t respawn_remaining{0};
    std::uint32_t deaths{0};
};

struct ShieldComponent {
    float value{0.0F};
};

struct CharacterMovementComponent {
    float spawn_x{0.0F};
    float spawn_z{0.0F};
    float spawn_y{0.0F};
    float facing_x{1.0F};
    float facing_z{0.0F};
};

struct WeaponComponent {
    LegacyArsenal arsenal;
    std::uint32_t cooldown_remaining{0};
    std::uint32_t next_fire_sequence{1};
    std::uint32_t shot_sequence{0};
    std::uint64_t shot_tick{0};
    std::array<float, 3> shot_impact{};
    bool shot_hit{false};
    bool shot_contact{false};
    bool shot_explosion{false};
    float aim_pitch{0};
    bool audio_guiding{};
};

struct AbilityComponent {
    std::uint32_t cooldown_remaining{0};
    std::uint32_t active_remaining{0};
    std::uint32_t secondary_cooldown_remaining{0};
    std::uint32_t secondary_active_remaining{0};
    std::uint32_t next_sequence{1};
    bool hit_consumed{false};
    float flash_factor{0.0F};
};

struct CharacterLoadoutComponent {
    SlicePlayerSelection selection;
};

enum class ComponentAuthority : std::uint8_t {
    server,
    predicted_owner,
    interpolated_remote,
};

struct AuthorityComponent {
    network::NetworkEntityId network_entity{0};
    network::ConnectionId connection{network::invalid_connection};
    ComponentAuthority mode{ComponentAuthority::server};
};

struct ReplicationComponent {
    std::uint64_t simulation_tick{0};
    std::uint64_t last_received_tick{0};
    std::uint32_t acknowledged_input{0};
    bool grounded{true};
};

struct CharacterPresentationComponent {
    std::uint32_t hit_marker_ticks{0};
};

struct ScoreComponent {
    std::uint32_t kills{0};
};

struct SliceCharacterDesc {
    network::NetworkEntityId network_entity{0};
    network::ConnectionId connection{network::invalid_connection};
    float spawn_x{0.0F};
    float spawn_z{0.0F};
    SlicePlayerSelection selection;
};

[[nodiscard]] inline core::EntityId
compose_slice_character(core::EntityRegistry& registry,
                        const SliceCharacterDesc description) {
    const auto entity = registry.create();
    registry.emplace<TransformComponent>(
        entity, TransformComponent{.position_x = description.spawn_x,
                                   .position_z = description.spawn_z});
    registry.emplace<CharacterPhysicsComponent>(entity);
    registry.emplace<HealthComponent>(entity);
    registry.emplace<ShieldComponent>(entity);
    registry.emplace<CharacterMovementComponent>(
        entity, CharacterMovementComponent{.spawn_x = description.spawn_x,
                                           .spawn_z = description.spawn_z});
    registry.emplace<WeaponComponent>(entity);
    registry.emplace<AbilityComponent>(entity);
    registry.emplace<CharacterLoadoutComponent>(
        entity, CharacterLoadoutComponent{.selection = description.selection});
    registry.emplace<AuthorityComponent>(
        entity, AuthorityComponent{.network_entity = description.network_entity,
                                   .connection = description.connection});
    registry.emplace<ReplicationComponent>(entity);
    registry.emplace<CharacterPresentationComponent>(entity);
    registry.emplace<ScoreComponent>(entity);
    return entity;
}

[[nodiscard]] inline bool has_complete_character_composition(
    const core::EntityRegistry& registry, const core::EntityId entity) noexcept {
    return registry.has<TransformComponent>(entity) &&
           registry.has<CharacterPhysicsComponent>(entity) &&
           registry.has<HealthComponent>(entity) &&
           registry.has<ShieldComponent>(entity) &&
           registry.has<CharacterMovementComponent>(entity) &&
           registry.has<WeaponComponent>(entity) &&
           registry.has<AbilityComponent>(entity) &&
           registry.has<CharacterLoadoutComponent>(entity) &&
           registry.has<AuthorityComponent>(entity) &&
           registry.has<ReplicationComponent>(entity) &&
           registry.has<CharacterPresentationComponent>(entity) &&
           registry.has<ScoreComponent>(entity);
}

} // namespace gloom::gameplay
