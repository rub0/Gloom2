#include <gloom/gameplay/combat_effects.hpp>

namespace gloom::gameplay {
void CombatEffects::reset(render::ParticleSystem& p) noexcept {
    p.clear();
    for (State& state : states_)
        state = {};
}
void CombatEffects::burst(render::ParticleSystem& particles, uint64 owner, const char* name, render::Vec3 position, uint64 seed) {
    particles.burst(owner, name, position, {0, 1, 0}, seed);
    ++events_;
}
void CombatEffects::observe(
    render::ParticleSystem& particles, const CombatantView& v, const CharacterAnimationFrame& frame, const render::Transform& parent, uint64 tick, bool fps) {
    State& state = states_[fps ? 0 : 1];
    if (state.valid && state.view.entity == v.entity && tick < state.tick)
        return;
    const render::Vec3 muzzle = render::attach_transform(parent, {.position = frame.muzzle}).position;
    const uint64 muzzle_owner = 0x8000000000000000ULL | v.entity;
    const render::Vec3 foot{v.position_x, v.position_y, v.position_z};
    const render::Vec3 chest{v.position_x, v.position_y + 1.1F, v.position_z};
    const render::Vec3 direction{v.facing_x, 0, v.facing_z};
    const bool baseline = !state.valid || state.view.entity != v.entity;
    if (baseline && state.valid) {
        particles.cancel(state.view.entity);
        particles.cancel(0x8000000000000000ULL | state.view.entity);
    }
    if (!baseline && state.muzzle_valid)
        state.muzzle_valid = particles.translate(muzzle_owner, {muzzle.x - state.muzzle.x, muzzle.y - state.muzzle.y, muzzle.z - state.muzzle.z});
    if (!baseline && tick > state.tick) {
        if (frame.cut || state.view.deaths != v.deaths || state.view.character != v.character) {
            particles.cancel(v.entity);
            particles.cancel(muzzle_owner);
        }
        if (!v.alive && state.view.alive)
            burst(particles, v.entity, "death", chest, v.entity * 1000003 + tick);
        if (v.alive && (!state.view.alive || state.view.deaths != v.deaths))
            burst(particles, v.entity, "spawn", foot, v.entity * 1000003 + tick);
        if (v.alive && !state.view.grounded && v.grounded)
            burst(particles, v.entity, "landing", foot, v.entity * 1000003 + tick);
        if (v.alive && v.life < state.view.life)
            burst(particles, v.entity, "damage", chest, v.entity * 1000003 + tick);
        if (v.alive && ((v.primary_ability_active && !state.view.primary_ability_active && v.ability == SliceAbility::guard) || v.shield < state.view.shield))
            burst(particles, v.entity, "shield", chest, v.entity * 1000003 + tick);
        if (v.alive && v.secondary_ability_active && !state.view.secondary_ability_active && v.secondary_ability == SliceSecondaryAbility::flash)
            burst(particles, v.entity, "shield", chest, v.entity * 1000003 + tick);
        if (v.alive && v.shot_sequence != state.view.shot_sequence && v.shot_tick <= tick && tick - v.shot_tick <= 15) {
            particles.burst(muzzle_owner, "muzzle", muzzle, direction, v.entity * 1000003 + v.shot_sequence, fps);
            ++events_;
            state.muzzle_valid = true;
            if (v.shot_contact) {
                particles.burst(v.entity, v.shot_explosion ? "explosion_review" : "impact", {v.shot_impact[0], v.shot_impact[1], v.shot_impact[2]},
                    {-direction.x, .2F, -direction.z}, v.entity * 1000003 + v.shot_sequence);
                ++events_;
            }
        }
    }
    if (!fps && v.alive) {
        if (v.character == SliceCharacter::shadow) {
            render::Vec3 tail = frame.tail;
            tail.y += .15F;
            particles.emitter(v.entity * 32 + 1, v.entity, "shadow_smoke", render::attach_transform(parent, {.position = tail}).position, {0, -1, 0});
            render::Vec3 head = frame.head;
            head.z += .20F;
            head.y += .085F;
            head.x -= .043F;
            particles.emitter(
                v.entity * 32 + 2, v.entity, "shadow_eyes", render::attach_transform(parent, {.position = head}).position, {-direction.x, 0, -direction.z});
            head.x += .086F;
            particles.emitter(
                v.entity * 32 + 3, v.entity, "shadow_eyes", render::attach_transform(parent, {.position = head}).position, {-direction.x, 0, -direction.z});
        } else {
            render::Vec3 left = frame.left_wing;
            left.x -= .28F;
            left.y += .14F;
            render::Vec3 right = frame.right_wing;
            right.x += .28F;
            right.y += .14F;
            left.z += .12F;
            right.z += .12F;
            particles.emitter(v.entity * 32 + 4, v.entity, "archangel_energy", render::attach_transform(parent, {.position = left}).position, {0, .3F, -.1F});
            particles.emitter(v.entity * 32 + 5, v.entity, "archangel_energy", render::attach_transform(parent, {.position = right}).position, {0, .3F, -.1F});
        }
    }
    state.view = v;
    state.tick = tick;
    state.muzzle = muzzle;
    state.valid = true;
}
} // namespace gloom::gameplay
