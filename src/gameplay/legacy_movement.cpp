#include <gloom/gameplay/legacy_movement.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace gloom::gameplay {
LegacyMotion legacy_motion(physics::Vec3 velocity,bool grounded,float x,float z,
    bool jump,bool dodge,float seconds,LegacyMovementProfile p) {
    if(!std::isfinite(seconds)||seconds<=0||seconds>.1F || !std::isfinite(x)||!std::isfinite(z) ||
       !std::isfinite(velocity.x)||!std::isfinite(velocity.y)||!std::isfinite(velocity.z))
        throw std::invalid_argument{"Invalid original movement step"};
    const float length=std::sqrt(x*x+z*z);
    if(length>0){x/=length;z/=length;}
    constexpr float units=legacy_unit_scale/legacy_motion_step;
    const float steps=seconds/legacy_motion_step;
    if(grounded && dodge && length>0){
        constexpr float diagonal=.7071067811865475F;
        velocity.x+=x*p.dodge_momentum.x*diagonal*units;
        velocity.z+=z*p.dodge_momentum.z*diagonal*units;
        velocity.y=-.007F*16.F*units+p.dodge_momentum.y*diagonal*units;grounded=false;
    }else if(grounded && jump){velocity.y=p.jump_momentum*units*normal_jump_scale;grounded=false;}
    if(grounded){
        const float retain=std::pow(length>0?p.maximum_momentum/(p.maximum_momentum+8.F*p.acceleration):.8F,steps);
        velocity.x=velocity.x*retain+x*p.maximum_momentum*units*(1-retain);
        velocity.z=velocity.z*retain+z*p.maximum_momentum*units*(1-retain);
        // Jolt's restored support and floor snap keep grounded motion attached
        // to slopes. A permanent downward impulse would create idle sliding.
        velocity.y=0;
    }else{
        const float retain=std::pow(.98F,steps);
        // Exact geometric-series extension of the original 0.98*v + a*2 rule.
        const float push=2.F*p.acceleration*units*(1-retain)/.02F;
        velocity.x=velocity.x*retain+x*push;velocity.z=velocity.z*retain+z*push;
        const float limit=p.maximum_momentum*6.F*units;
        velocity.y=std::clamp(velocity.y+legacy_gravity*seconds,-limit,limit);
    }
    return {velocity,!grounded};
}
}
