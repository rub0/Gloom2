#pragma once

#include <gloom/gameplay/slice_selection.hpp>

#include <cstddef>
#include <string_view>

namespace gloom::gameplay {

struct CookedPresentationSlot {
    std::string_view source_uri;
    std::string_view cooked_uri;
    std::size_t required_instances{0};
};

inline constexpr CookedPresentationSlot soul_reaper_presentation{
    .source_uri = "game:/weapons/soul_reaper.gltf",
    .cooked_uri = "cache:/weapons/soul_reaper.gasset",
    .required_instances = 2,
};

inline constexpr CookedPresentationSlot original_soul_reaper_presentation{
    .source_uri = "game:/characters/original/soul_reaper.gltf",
    .cooked_uri = "cache:/characters/original/soul_reaper.gasset",
    .required_instances = 1,
};

[[nodiscard]] constexpr CookedPresentationSlot original_weapon_presentation(SliceWeapon weapon) noexcept {
    switch(weapon){
    case SliceWeapon::soul_reaper:return original_soul_reaper_presentation;
    case SliceWeapon::sniper:return {"game:/legacy/sniper.gltf","cache:/legacy/sniper.gasset",1};
    case SliceWeapon::shotgun:return {"game:/legacy/shotgun.gltf","cache:/legacy/shotgun.gasset",1};
    case SliceWeapon::minigun:return {"game:/legacy/minigun.gltf","cache:/legacy/minigun.gasset",1};
    case SliceWeapon::iron_hell_goat:return {"game:/legacy/iron_hell_goat.gltf","cache:/legacy/iron_hell_goat.gasset",1};
    }
    return original_soul_reaper_presentation;
}

inline constexpr CookedPresentationSlot hound_ability_presentation{
    .source_uri = "game:/abilities/hound_abilities.gltf",
    .cooked_uri = "cache:/abilities/hound_abilities.gasset",
    .required_instances = 2,
};

} // namespace gloom::gameplay
