#pragma once

#include <gloom/gameplay/slice_selection.hpp>

#include <array>
#include <algorithm>
#include <cstdint>
#include <span>

namespace gloom::gameplay {

inline constexpr float legacy_gameplay_scale = 0.15F;

struct LegacyWeaponRule {
    SliceWeapon weapon{};
    std::uint16_t maximum_ammo{};
    std::uint16_t primary_ammo{};
    std::uint16_t secondary_ammo{};
    std::uint16_t maximum_charge_ammo{};
    std::uint16_t projectile_count{};
    std::uint16_t primary_cooldown_ticks{};
    std::uint16_t secondary_cooldown_ticks{};
    std::uint16_t charge_ticks{};
    float primary_damage{};
    float secondary_damage{};
    float range{};
    float dispersion_degrees{};
    float projectile_radius{};
    float projectile_speed{};
    float explosion_radius{};
};

[[nodiscard]] const LegacyWeaponRule& legacy_weapon_rule(SliceWeapon weapon);
[[nodiscard]] const char* legacy_weapon_name(SliceWeapon weapon) noexcept;

struct LegacyArsenalInput {
    bool primary{};
    bool secondary{};
};

enum class LegacyWeaponActionKind : std::uint8_t {
    hitscan,
    expansive_hitscan,
    magnetic_projectiles,
    recall_projectiles,
    charged_hitscan,
    charged_fireball,
    pull_item,
    steer_fireballs,
};

struct LegacyWeaponAction {
    LegacyWeaponActionKind kind{};
    SliceWeapon weapon{};
    std::uint16_t projectile_count{};
    std::uint16_t ammo_spent{};
    float damage{};
    float range{};
    float dispersion_degrees{};
    float projectile_radius{};
    float projectile_speed{};
    float explosion_radius{};
    float charge_fraction{};
};

class LegacyArsenal final {
public:
    LegacyArsenal();
    void reset(SliceWeapon initial_weapon = SliceWeapon::soul_reaper) noexcept;
    [[nodiscard]] bool acquire(SliceWeapon weapon, std::uint16_t ammo) noexcept;
    [[nodiscard]] bool add_ammo(SliceWeapon weapon, std::uint16_t ammo) noexcept;
    [[nodiscard]] bool select(SliceWeapon weapon) noexcept;
    [[nodiscard]] SliceWeapon active_weapon() const noexcept;
    [[nodiscard]] bool owns(SliceWeapon weapon) const noexcept;
    [[nodiscard]] std::uint16_t ammo(SliceWeapon weapon) const noexcept;
    [[nodiscard]] std::uint16_t active_ammo() const noexcept;
    [[nodiscard]] std::uint16_t cooldown_remaining() const noexcept;
    [[nodiscard]] float charge_fraction() const noexcept;
    void activate_damage_modifier(std::uint16_t percent) noexcept { damage_percent_=percent; damage_ticks_=900; }
    void activate_cooldown_modifier(std::uint16_t percent) noexcept { cooldown_percent_=std::min<std::uint16_t>(percent,100); modifier_ticks_=900; }
    [[nodiscard]] std::uint16_t damage_modifier_ticks() const noexcept { return damage_ticks_; }
    [[nodiscard]] std::uint16_t cooldown_modifier_ticks() const noexcept { return modifier_ticks_; }
    void clear_modifiers() noexcept { damage_ticks_=modifier_ticks_=damage_percent_=cooldown_percent_=0; }
    [[nodiscard]] std::span<const LegacyWeaponAction> tick(LegacyArsenalInput input) noexcept;

private:
    void emit(LegacyWeaponAction action) noexcept;
    [[nodiscard]] bool spend(std::uint16_t amount) noexcept;
    std::array<bool, slice_weapon_count> owned_{};
    std::array<std::uint16_t, slice_weapon_count> ammo_{};
    std::array<LegacyWeaponAction, 12> actions_{};
    std::size_t action_count_{};
    SliceWeapon active_{SliceWeapon::soul_reaper};
    std::uint16_t cooldown_{};
    std::uint16_t charge_ticks_{};
    std::uint16_t charge_ammo_{};
    bool previous_primary_{};
    bool previous_secondary_{};
    bool magnetic_projectiles_{};
    bool controllable_fireballs_{};
    float minigun_dispersion_{1.5F};
    std::uint16_t damage_ticks_{},modifier_ticks_{},damage_percent_{},cooldown_percent_{};
};

} // namespace gloom::gameplay
