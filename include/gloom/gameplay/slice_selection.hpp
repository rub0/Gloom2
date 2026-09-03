#pragma once

#include <cstdint>
#include <cstddef>

namespace gloom::gameplay {

// Value 1 remains a compatibility slot for the old Hound/Berserker prototype.
// It is not offered as an independent original class.
enum class SliceCharacter : std::uint8_t { hound = 0, berserker = 1, archangel = 2, shadow = 3 };
inline constexpr std::size_t slice_character_count = 4;
enum class SliceWeapon : std::uint8_t { soul_reaper = 0 };
enum class SliceAbility : std::uint8_t { bite = 0, none = 1, guard = 2 };

struct SlicePlayerSelection {
    SliceCharacter character{SliceCharacter::hound};
    SliceWeapon weapon{SliceWeapon::soul_reaper};
    SliceAbility ability{SliceAbility::bite};

    [[nodiscard]] bool operator==(const SlicePlayerSelection&) const noexcept = default;
};

[[nodiscard]] constexpr bool valid_slice_selection(
    const SlicePlayerSelection selection) noexcept {
    if (selection.weapon != SliceWeapon::soul_reaper) {
        return false;
    }
    if (selection.character == SliceCharacter::berserker || selection.character == SliceCharacter::archangel ||
        selection.character == SliceCharacter::shadow) {
        return selection.ability == SliceAbility::none;
    }
    return selection.character == SliceCharacter::hound &&
           (selection.ability == SliceAbility::bite ||
            selection.ability == SliceAbility::none ||
            selection.ability == SliceAbility::guard);
}

} // namespace gloom::gameplay
