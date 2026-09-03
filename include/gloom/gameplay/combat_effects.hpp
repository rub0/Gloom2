#pragma once
#include <gloom/gameplay/character_animation.hpp>
#include <gloom/render/particles.hpp>
#include <array>

namespace gloom::gameplay {
// Confirmed snapshot events only: speculative fire commands never emit an
// impact or a second muzzle. A new connection establishes a fresh baseline.
class CombatEffects {
public:
    void observe(render::ParticleSystem& particles,const CombatantView& view,
        const CharacterAnimationFrame& frame,const render::Transform& parent,
        std::uint64_t tick,bool first_person);
    void reset(render::ParticleSystem& particles) noexcept;
    [[nodiscard]] std::uint64_t events() const noexcept {return events_;}
private:
    struct State {CombatantView view;std::uint64_t tick{0};bool valid{false};};
    std::array<State,2> states_;
    std::uint64_t events_{0};
};
} // namespace gloom::gameplay
