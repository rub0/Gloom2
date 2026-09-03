#pragma once

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

inline constexpr CookedPresentationSlot hound_ability_presentation{
    .source_uri = "game:/abilities/hound_abilities.gltf",
    .cooked_uri = "cache:/abilities/hound_abilities.gasset",
    .required_instances = 2,
};

} // namespace gloom::gameplay
