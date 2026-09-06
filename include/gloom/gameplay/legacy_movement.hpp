#pragma once
#include <gloom/gameplay/slice_selection.hpp>
#include <gloom/physics/world.hpp>

namespace gloom::gameplay {
// AvatarController momentum is displacement per 16 ms, in original map units.
inline constexpr float legacy_unit_scale = .15F;
inline constexpr float legacy_motion_step = .016F;
inline constexpr float normal_jump_scale = .9F;
struct LegacyMovementProfile {
    float acceleration, maximum_momentum, jump_momentum;
    physics::Vec3 dodge_momentum;
};
[[nodiscard]] constexpr LegacyMovementProfile legacy_movement_profile(SliceCharacter character) {
    switch(character) {
    case SliceCharacter::hound: case SliceCharacter::berserker: return {.017F,1.3F,1.8F,{2.5F,2.F,2.5F}};
    case SliceCharacter::archangel: return {.015F,.8F,1.4F,{1.7F,1.9F,1.7F}};
    default: return {.015F,.9F,1.5F,{2.F,2.F,2.F}};
    }
}
struct LegacyMotion {
    physics::Vec3 velocity;
    bool airborne;
};
// Pure shared authority/prediction step. Collision resolution remains in Jolt.
[[nodiscard]] LegacyMotion legacy_motion(physics::Vec3 velocity, bool grounded,
    float axis_x,float axis_z,bool jump,bool dodge,float seconds,LegacyMovementProfile profile);
inline constexpr float legacy_contact_velocity = -.15F * legacy_unit_scale / legacy_motion_step;
inline constexpr float legacy_gravity = -.007F * 8.F * legacy_unit_scale / (legacy_motion_step*legacy_motion_step);
}
