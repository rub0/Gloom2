#pragma once
#include <gloom/gameplay/character_animation.hpp>
#include <gloom/render/particles.hpp>

namespace gloom::gameplay {
// Confirmed snapshot events only: speculative fire commands never emit an
// impact or a second muzzle. A new connection establishes a fresh baseline.
class CombatEffects {
  public:
    void observe(render::ParticleSystem& particles, const CombatantView& view, const CharacterAnimationFrame& frame, const render::Transform& parent,
        uint64 tick, bool first_person);
    void reset(render::ParticleSystem& particles) noexcept;
    [[nodiscard]] uint64 events() const noexcept {
        return events_;
    }

  private:
    struct State {
        CombatantView view;
        uint64 tick{0};
        render::Vec3 muzzle;
        bool muzzle_valid{false}, valid{false};
    };
    void burst(render::ParticleSystem& particles, uint64 owner, const char* name, render::Vec3 position, uint64 seed);
    State states_[2]{};
    uint64 events_{0};
};
} // namespace gloom::gameplay
