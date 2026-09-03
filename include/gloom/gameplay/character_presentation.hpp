#pragma once

#include <gloom/gameplay/slice_selection.hpp>

#include <array>
#include <span>
#include <string_view>

namespace gloom::gameplay {

struct CharacterPresentationPart {
    float forward_offset{0.0F};
    float right_offset{0.0F};
    float height{0.0F};
    float scale_x{0.0F};
    float scale_y{0.0F};
    float scale_z{0.0F};
    float red{1.0F};
    float green{1.0F};
    float blue{1.0F};
    bool ability_tint{false};
};

struct CharacterPresentationRecipe {
    SliceCharacter character{SliceCharacter::hound};
    std::string_view authored_scene_uri;
    std::string_view cooked_scene_uri;
    std::span<const CharacterPresentationPart> fallback_parts;
};

inline constexpr std::array hound_presentation_parts{
    CharacterPresentationPart{0.0F, 0.0F, 0.92F, 0.48F, 0.52F, 0.68F,
                              0.24F, 0.07F, 0.055F, true},
    CharacterPresentationPart{0.58F, 0.0F, 1.35F, 0.38F, 0.34F, 0.38F,
                              0.62F, 0.13F, 0.055F},
    CharacterPresentationPart{0.94F, 0.0F, 1.24F, 0.23F, 0.18F, 0.32F,
                              0.14F, 0.025F, 0.02F},
    CharacterPresentationPart{0.48F, -0.29F, 1.72F, 0.10F, 0.27F, 0.10F,
                              0.24F, 0.07F, 0.055F, true},
    CharacterPresentationPart{0.48F, 0.29F, 1.72F, 0.10F, 0.27F, 0.10F,
                              0.24F, 0.07F, 0.055F, true},
    CharacterPresentationPart{0.25F, -0.32F, 0.35F, 0.15F, 0.35F, 0.15F,
                              0.62F, 0.13F, 0.055F},
    CharacterPresentationPart{0.25F, 0.32F, 0.35F, 0.15F, 0.35F, 0.15F,
                              0.62F, 0.13F, 0.055F},
    CharacterPresentationPart{-0.38F, -0.32F, 0.35F, 0.15F, 0.35F, 0.15F,
                              0.24F, 0.07F, 0.055F, true},
    CharacterPresentationPart{-0.38F, 0.32F, 0.35F, 0.15F, 0.35F, 0.15F,
                              0.24F, 0.07F, 0.055F, true},
};

inline constexpr std::array berserker_presentation_parts{
    CharacterPresentationPart{0.0F, 0.0F, 1.08F, 0.42F, 0.55F, 0.25F,
                              0.18F, 0.20F, 0.24F},
    CharacterPresentationPart{0.0F, 0.0F, 1.68F, 0.27F, 0.27F, 0.27F,
                              0.46F, 0.32F, 0.24F},
    CharacterPresentationPart{0.0F, -0.48F, 1.36F, 0.18F, 0.22F, 0.20F,
                              0.34F, 0.09F, 0.045F},
    CharacterPresentationPart{0.0F, 0.48F, 1.36F, 0.18F, 0.22F, 0.20F,
                              0.34F, 0.09F, 0.045F},
    CharacterPresentationPart{0.0F, -0.48F, 0.93F, 0.14F, 0.36F, 0.14F,
                              0.46F, 0.32F, 0.24F},
    CharacterPresentationPart{0.0F, 0.48F, 0.93F, 0.14F, 0.36F, 0.14F,
                              0.46F, 0.32F, 0.24F},
    CharacterPresentationPart{0.0F, -0.22F, 0.35F, 0.18F, 0.38F, 0.18F,
                              0.12F, 0.13F, 0.16F},
    CharacterPresentationPart{0.0F, 0.22F, 0.35F, 0.18F, 0.38F, 0.18F,
                              0.12F, 0.13F, 0.16F},
    CharacterPresentationPart{0.27F, 0.0F, 1.22F, 0.06F, 0.34F, 0.22F,
                              0.62F, 0.18F, 0.055F},
};

[[nodiscard]] constexpr CharacterPresentationRecipe character_presentation_recipe(
    const SliceCharacter character, const bool original = false) noexcept {
    if (original) {
        if (character==SliceCharacter::shadow)
            return {.character=character,.authored_scene_uri="game:/characters/original/shadow.gltf",
                    .cooked_scene_uri="cache:/characters/original/shadow.gasset",.fallback_parts=berserker_presentation_parts};
        return {.character=character,.authored_scene_uri="game:/characters/original/archangel.gltf",
                .cooked_scene_uri="cache:/characters/original/archangel.gasset",.fallback_parts=berserker_presentation_parts};
    }
    if (character == SliceCharacter::berserker) {
        return {.character = character,
                .authored_scene_uri = "game:/characters/berserker.gltf",
                .cooked_scene_uri = "cache:/characters/berserker.gasset",
                .fallback_parts = berserker_presentation_parts};
    }
    return {.character = SliceCharacter::hound,
            .authored_scene_uri = "game:/characters/hound.gltf",
            .cooked_scene_uri = "cache:/characters/hound.gasset",
            .fallback_parts = hound_presentation_parts};
}

} // namespace gloom::gameplay
