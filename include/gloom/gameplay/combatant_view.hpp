#pragma once
#include <gloom/gameplay/slice_selection.hpp>
#include <gloom/core/types.hpp>

namespace gloom::gameplay {
inline constexpr float legacy_default_life = 120.0F;
struct CombatantView {
    uint64 entity{0};
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
    uint32 kills{0};
    uint32 deaths{0};
    uint32 current_spree{0};
    SliceCharacter character{SliceCharacter::hound};
    SliceWeapon weapon{SliceWeapon::soul_reaper};
    SliceAbility ability{SliceAbility::bite};
    SliceSecondaryAbility secondary_ability{SliceSecondaryAbility::berserker};
    bool primary_ability_active{false};
    bool secondary_ability_active{false};
    float flash_factor{0.0F};
    bool alive{true};
    bool grounded{true};
    bool air_dodge_available{false};
    float aim_pitch{0};
    uint32 shot_sequence{0};
    uint64 shot_tick{0};
    float shot_impact[3]{};
    bool shot_hit{false};
    bool shot_contact{false};
    bool shot_explosion{false};
    uint16 ammunition[slice_weapon_count]{};
    uint8 owned_weapons{1};
    float weapon_charge_fraction{0};
    uint16 damage_modifier_ticks{};
    uint16 cooldown_modifier_ticks{};
    bool audio_guiding{};
};

}
