#pragma once
#include <gloom/assets/animation.hpp>
#include <gloom/gameplay/vertical_slice.hpp>

namespace gloom::gameplay {
struct CharacterAnimationFrame {
    assets::LocalPose local;
    std::vector<assets::RigMatrix> worlds;
    render::Transform weapon;
    render::Vec3 muzzle, left_grip, head, tail, left_wing, right_wing;
    bool cut{true};
};

// Presentation only. The input is an interpolated view, never an authority output.
class CharacterAnimator {
public:
    CharacterAnimationFrame update(const assets::ImportedScene& rig, const CombatantView& view,
                                    double seconds, bool discontinuity = false, bool bind = false,
                                    bool first_person = false);
    void reset() noexcept;
private:
    CombatantView previous_;
    assets::LocalPose last_pose_,death_pose_;
    bool initialized_{false};
    double time_{0}, air_time_{0}, death_time_{0}, equip_time_{0};
    float movement_{0}, landing_{0}, recoil_{0}, damage_{0};
};
} // namespace gloom::gameplay
