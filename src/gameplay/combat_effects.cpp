#include <gloom/gameplay/combat_effects.hpp>

namespace gloom::gameplay {
void CombatEffects::reset(render::ParticleSystem& p) noexcept {p.clear();states_={};}
void CombatEffects::observe(render::ParticleSystem& particles,const CombatantView& v,
    const CharacterAnimationFrame& frame,const render::Transform& parent,std::uint64_t tick,bool fps) {
    auto& state=states_[fps?0:1];
    if (state.valid && state.view.entity==v.entity && tick<state.tick) return;
    const auto point=[&](render::Vec3 p){return render::attach_transform(parent,{.position=p}).position;};
    const render::Vec3 foot{v.position_x,v.position_y,v.position_z};
    const render::Vec3 chest{v.position_x,v.position_y+1.1F,v.position_z};
    const auto direction=render::Vec3{v.facing_x,0,v.facing_z};
    const bool baseline=!state.valid || state.view.entity!=v.entity;
    if (baseline && state.valid) particles.cancel(state.view.entity);
    if (!baseline && tick>state.tick) {
        const auto burst=[&](std::string_view name,render::Vec3 position,bool view_model=false) {
            particles.burst(v.entity,name,position,{0,1,0},v.entity*1000003+tick,view_model);++events_;
        };
        if (frame.cut || state.view.deaths!=v.deaths || state.view.character!=v.character) particles.cancel(v.entity);
        if (!v.alive && state.view.alive) burst("death",chest);
        if (v.alive && (!state.view.alive || state.view.deaths!=v.deaths)) burst("spawn",foot);
        if (v.alive && !state.view.grounded && v.grounded) burst("landing",foot);
        if (v.alive && v.life<state.view.life) burst("damage",chest);
        if (v.alive && ((v.primary_ability_active && !state.view.primary_ability_active && v.ability==SliceAbility::guard) || v.shield<state.view.shield)) burst("shield",chest);
        if (v.alive && v.shot_sequence!=state.view.shot_sequence && v.shot_tick<=tick && tick-v.shot_tick<=15) {
            particles.burst(v.entity,"muzzle",point(frame.muzzle),direction,v.entity*1000003+v.shot_sequence,fps);++events_;
            if (v.shot_contact) {
                particles.burst(v.entity,v.shot_explosion?"explosion_review":"impact",{v.shot_impact[0],v.shot_impact[1],v.shot_impact[2]},
                    {-direction.x,.2F,-direction.z},v.entity*1000003+v.shot_sequence);++events_;
            }
        }
    }
    if (!fps && v.alive) {
        const auto emitter=[&](std::uint64_t socket,std::string_view name,render::Vec3 p,render::Vec3 d){
            particles.emitter(v.entity*32+socket,v.entity,name,point(p),d);
        };
        if (v.character==SliceCharacter::shadow) {
            auto tail=frame.tail;tail.y+=.15F;
            emitter(1,"shadow_smoke",tail,{0,-1,0});
            auto head=frame.head;head.z+=.20F;head.y+=.085F;
            head.x-=.043F;emitter(2,"shadow_eyes",head,{-direction.x,0,-direction.z});
            head.x+=.086F;emitter(3,"shadow_eyes",head,{-direction.x,0,-direction.z});
        } else {
            auto left=frame.left_wing;left.x-=.28F;left.y+=.14F;
            auto right=frame.right_wing;right.x+=.28F;right.y+=.14F;
            left.z+=.12F;right.z+=.12F;
            emitter(4,"archangel_energy",left,{0,.3F,-.1F});
            emitter(5,"archangel_energy",right,{0,.3F,-.1F});
        }
    }
    state={v,tick,true};
}
} // namespace gloom::gameplay
