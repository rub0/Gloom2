#pragma once
#include <gloom/assets/animation.hpp>
#include <gloom/gameplay/combatant_view.hpp>

namespace gloom::gameplay {
struct CharacterAnimationFrame {
    Span<const render::Transform> local;
    Span<const assets::RigMatrix> worlds;
    Span<const render::SkinPose> skins;
    render::Transform weapon;
    render::Vec3 muzzle, left_grip, head, tail, left_wing, right_wing;
    bool cut{true};
    uint64 actor{0}, generation{0}, identity{0};
};

// Presentation only. The input is an interpolated view, never an authority output.
class CharacterAnimator {
  public:
    // Bind a nonempty prepared rig after the preceding frame's CPU consumers finish. Each animator belongs to one actor.
    void bind(const assets::AnimationRig& rig);
    // Returned views survive through this frame's end_frame, but expire on the next update/bind/reset/destruction.
    // Skin storage from the preceding update stays immutable through this update's end_frame.
    const CharacterAnimationFrame& update(
        const assets::AnimationRig& rig, const CombatantView& view, double seconds, bool discontinuity = false, bool rest = false, bool first_person = false);
    const CharacterAnimationFrame& sample(const assets::AnimationRig& rig, uint64 actor, const char* clip, double seconds);
    void reset() noexcept;

  private:
    CombatantView previous_;
    assets::LocalPose last_pose_, death_pose_;
    assets::LocalPose local_, scratch_[3];
    Array<assets::RigMatrix> worlds_;
    Array<render::SkinPose> skins_[2];
    CharacterAnimationFrame frame_;
    const assets::AnimationRig* rig_{nullptr};
    uint64 generation_{0}, identity_{0};
    uint32 skin_slot_{0};
    void finish_pose(const assets::AnimationRig& rig);
    bool initialized_{false};
    bool sampled_{false};
    double time_{0}, air_time_{0}, death_time_{0}, equip_time_{0};
    float movement_{0}, landing_{0}, recoil_{0}, damage_{0};
};
} // namespace gloom::gameplay
