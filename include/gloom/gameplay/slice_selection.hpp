#pragma once

#include <gloom/core/types.hpp>
#include <stddef.h>

namespace gloom::gameplay {

// Value 1 remains a compatibility slot for the old Hound/Berserker prototype.
// It is not offered as an independent original class.
enum class SliceCharacter : uint8 { hound = 0, berserker = 1, archangel = 2, shadow = 3 };
inline constexpr size_t slice_character_count = 4;
enum class SliceWeapon : uint8 {
    soul_reaper = 0,
    sniper = 1,
    shotgun = 2,
    minigun = 3,
    iron_hell_goat = 4,
};
inline constexpr size_t slice_weapon_count = 5;
enum class SliceAbility : uint8 {
    bite = 0,
    none = 1,
    guard = 2,
    diamond_skin = 3,
    invisibility = 4,
};

enum class SliceSecondaryAbility : uint8 {
    none = 0,
    berserker = 1,
    life_dome = 2,
    flash = 3,
};

struct SlicePlayerSelection {
    SliceCharacter character{SliceCharacter::hound};
    SliceWeapon weapon{SliceWeapon::soul_reaper};
    SliceAbility ability{SliceAbility::bite};

    [[nodiscard]] bool operator==(const SlicePlayerSelection&) const noexcept = default;
};

[[nodiscard]] constexpr bool valid_slice_selection(const SlicePlayerSelection selection) noexcept {
    if (static_cast<size_t>(selection.weapon) >= slice_weapon_count) {
        return false;
    }
    if (selection.character == SliceCharacter::berserker) {
        return selection.ability == SliceAbility::none;
    }
    if (selection.character == SliceCharacter::archangel)
        return selection.ability == SliceAbility::diamond_skin;
    if (selection.character == SliceCharacter::shadow)
        return selection.ability == SliceAbility::invisibility;
    return selection.character == SliceCharacter::hound &&
           (selection.ability == SliceAbility::bite || selection.ability == SliceAbility::none || selection.ability == SliceAbility::guard);
}

[[nodiscard]] constexpr SliceSecondaryAbility secondary_ability(const SlicePlayerSelection selection) noexcept {
    if (selection.character == SliceCharacter::hound && selection.ability != SliceAbility::none)
        return SliceSecondaryAbility::berserker;
    if (selection.character == SliceCharacter::archangel)
        return SliceSecondaryAbility::life_dome;
    if (selection.character == SliceCharacter::shadow)
        return SliceSecondaryAbility::flash;
    return SliceSecondaryAbility::none;
}

} // namespace gloom::gameplay
