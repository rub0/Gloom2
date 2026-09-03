#pragma once

#include <gloom/gameplay/slice_selection.hpp>
#include <gloom/network/movement_replication.hpp>
#include <gloom/physics/world.hpp>

#include <cstdint>
#include <array>
#include <memory>
#include <span>

namespace gloom::gameplay {

inline constexpr float legacy_default_life = 120.0F;
inline constexpr float legacy_maximum_life = 250.0F;
inline constexpr float legacy_maximum_shield = 150.0F;
inline constexpr float legacy_shield_absorption = 0.70F;
inline constexpr float soul_reaper_damage = 80.0F;
inline constexpr float soul_reaper_range = 15.0F;
inline constexpr double soul_reaper_cooldown_seconds = 0.5;
inline constexpr double legacy_respawn_seconds = 4.0;
inline constexpr float hound_bite_damage = 40.0F;
inline constexpr float hound_bite_life_steal = 40.0F;
inline constexpr float hound_bite_range = 1.65F;
inline constexpr double hound_bite_duration_seconds = 0.5;
inline constexpr double hound_bite_cooldown_seconds = 25.0;
inline constexpr float hound_guard_shield = 50.0F;
inline constexpr double hound_guard_duration_seconds = 1.0;
inline constexpr double hound_guard_cooldown_seconds = 15.0;

enum class ArenaMaterial : std::uint8_t {
    floor,
    industrial,
    trim,
    lava,
};

struct SliceInput {
    float axis_x{0.0F};
    float axis_z{0.0F};
    float aim_x{0.0F};
    float aim_y{0.0F};
    float aim_z{0.0F};
    bool jump{false};
    bool fire_primary{false};
    bool use_primary_ability{false};
};

struct ArenaBox {
    float center_x{0.0F};
    float center_y{0.0F};
    float center_z{0.0F};
    float half_x{0.5F};
    float half_y{0.5F};
    float half_z{0.5F};
    ArenaMaterial material{ArenaMaterial::industrial};
    bool solid{false};
};

struct ArenaSurface {
    float center_x{0.0F};
    float center_y{0.0F};
    float center_z{0.0F};
    float half_x{0.5F};
    float half_z{0.5F};
    ArenaMaterial material{ArenaMaterial::lava};
};

struct VerticalSliceSettings {
    bool opponent_ai_enabled{true};
    physics::World* authoritative_physics{nullptr};
    bool original_factory{false};
};

struct CombatantView {
    network::NetworkEntityId entity{0};
    float position_x{0.0F};
    float position_y{0.0F};
    float position_z{0.0F};
    float velocity_x{0.0F};
    float velocity_y{0.0F};
    float velocity_z{0.0F};
    float life{legacy_default_life};
    float shield{0.0F};
    float respawn_remaining_seconds{0.0F};
    float facing_x{1.0F};
    float facing_z{0.0F};
    std::uint32_t kills{0};
    std::uint32_t deaths{0};
    SliceCharacter character{SliceCharacter::hound};
    SliceWeapon weapon{SliceWeapon::soul_reaper};
    SliceAbility ability{SliceAbility::bite};
    bool primary_ability_active{false};
    bool alive{true};
    bool grounded{true};
    float aim_pitch{0};
    std::uint32_t shot_sequence{0};
    std::uint64_t shot_tick{0};
    std::array<float, 3> shot_impact{};
    bool shot_hit{false};
    bool shot_contact{false};
};

struct SliceHud {
    float life_fraction{1.0F};
    float shield_fraction{0.0F};
    float weapon_ready_fraction{1.0F};
    float primary_ability_ready_fraction{1.0F};
    bool primary_ability_active{false};
    bool hit_marker{false};
    bool dead{false};
    std::uint32_t kills{0};
    std::uint32_t deaths{0};
};

struct SliceNetworkMetrics {
    std::uint64_t active_sessions{0};
    std::uint64_t authorized_input_batches{0};
    std::uint64_t authorized_fire_commands{0};
    std::uint64_t authorized_ability_commands{0};
    std::uint64_t rejected_commands{0};
    std::uint64_t reconciliation_count{0};
};

struct KinematicMechanismView {
    network::NetworkEntityId entity{0};
    float position_x{0.0F};
    float position_y{0.0F};
    float position_z{0.0F};
    float velocity_x{0.0F};
    float velocity_y{0.0F};
    float velocity_z{0.0F};
};

struct SliceSnapshot {
    std::uint64_t simulation_tick{0};
    CombatantView player;
    CombatantView opponent;
    KinematicMechanismView factory_lift;
    SliceHud hud;
    SliceNetworkMetrics network;
    std::uint32_t scene_id{0};
};

struct SlicePresentationFeedbackState {
    float opponent_damage_remaining_seconds{0.0F};
    float opponent_death_remaining_seconds{0.0F};
    float opponent_respawn_remaining_seconds{0.0F};
};

// Converts authoritative snapshot transitions into short-lived, renderer-neutral
// presentation cues. It never predicts or mutates combat state.
class SlicePresentationFeedback final {
public:
    void observe(const SliceSnapshot& snapshot) noexcept;
    void advance(double elapsed_seconds) noexcept;
    [[nodiscard]] const SlicePresentationFeedbackState& state() const noexcept;

private:
    SlicePresentationFeedbackState state_;
    std::uint64_t last_tick_{0};
    network::NetworkEntityId opponent_entity_{0};
    float opponent_life_{0.0F};
    bool opponent_alive_{false};
    bool initialized_{false};
};

// A fixed-tick, backend-neutral gameplay seam. Platform input, physics and rendering
// are adapters around this class; none of those implementation APIs leak into it.
class VerticalSliceSimulation final {
public:
    static constexpr std::uint32_t tick_rate = 60;
    static constexpr network::NetworkEntityId player_entity = 1;
    static constexpr network::NetworkEntityId opponent_entity = 2;
    [[nodiscard]] static network::ReplicationSettings default_movement_settings();

    explicit VerticalSliceSimulation(bool opponent_ai_enabled = true);
    explicit VerticalSliceSimulation(VerticalSliceSettings settings);
    ~VerticalSliceSimulation();
    VerticalSliceSimulation(const VerticalSliceSimulation&) = delete;
    VerticalSliceSimulation& operator=(const VerticalSliceSimulation&) = delete;
    VerticalSliceSimulation(VerticalSliceSimulation&&) = delete;
    VerticalSliceSimulation& operator=(VerticalSliceSimulation&&) = delete;

    void tick(const SliceInput& input);
    void tick(const SliceInput& player_input, const SliceInput& opponent_input);
    void add_shield(network::NetworkEntityId entity, float amount);
    [[nodiscard]] bool set_selection(network::NetworkEntityId entity,
                                     SlicePlayerSelection selection);
    [[nodiscard]] const SliceSnapshot& snapshot() const noexcept;
    [[nodiscard]] SliceSnapshot
    snapshot_for(network::NetworkEntityId controlled_entity) const;
    [[nodiscard]] std::span<const ArenaBox> arena() const noexcept;
    [[nodiscard]] std::span<const ArenaSurface> surfaces() const noexcept;
    [[nodiscard]] const network::ReplicationSettings& movement_settings() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gloom::gameplay
