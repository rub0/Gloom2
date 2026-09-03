#pragma once
#include <gloom/gameplay/vertical_slice.hpp>
#include <cmath>

namespace gloom::review {
// Four ten-second shots: each original character in inspection/game lighting.
// Synthetic presentation inputs; real authority/transport acceptance is separate.
inline void animate_snapshot(gameplay::SliceSnapshot& s,std::size_t frame,int fps) {
    const auto segment=frame/static_cast<std::size_t>(fps*10);
    const double t=static_cast<double>(frame%static_cast<std::size_t>(fps*10))/fps;
    const auto character=segment%2==0?gameplay::SliceCharacter::archangel:gameplay::SliceCharacter::shadow;
    s.simulation_tick=static_cast<std::uint64_t>(static_cast<double>(frame)*60/fps)+1;
    s.player.character=s.opponent.character=character;
    s.player.ability=s.opponent.ability=gameplay::SliceAbility::none;
    s.opponent.facing_x=0;s.opponent.facing_z=1;
    s.opponent.velocity_x=s.opponent.velocity_y=s.opponent.velocity_z=0;
    if(t>=1 && t<2.2){s.opponent.velocity_z=3;s.opponent.position_z+=static_cast<float>((t-1)*.45);}
    if(t>=2.2 && t<3){s.opponent.velocity_z=-3;s.opponent.position_z+=static_cast<float>((3-t)*.675);}
    if(t>=3 && t<3.7){s.opponent.velocity_x=3;s.opponent.position_x+=static_cast<float>(std::sin((t-3)*4.488)*.35);}
    s.opponent.grounded=!(t>=3.7 && t<4.4);
    if(!s.opponent.grounded){s.opponent.position_y+=static_cast<float>(std::sin((t-3.7)*4.488)*.45);s.opponent.velocity_y=static_cast<float>(std::cos((t-3.7)*4.488)*2);}
    s.opponent.alive=!(t>=5 && t<9);s.opponent.life=s.opponent.alive?(t>=4.7 && t<5?40.0F:120.0F):0;
    s.opponent.deaths=t>=5?1:0;s.opponent.respawn_remaining_seconds=s.opponent.alive?0:static_cast<float>(9-t);
    s.opponent.aim_pitch=static_cast<float>(std::sin(t*2)*.20);
    s.opponent.shield=t>=4.55 && t<4.7?20.0F:t>=4.4 && t<4.55?50.0F:0.0F;
    s.player.aim_pitch=0;s.player.shot_sequence=s.opponent.shot_sequence=t>=4.5?1:0;
    s.player.shot_tick=s.opponent.shot_tick=static_cast<std::uint64_t>(segment*600+271);
    s.player.shot_contact=s.opponent.shot_contact=t>=4.5;
    s.player.shot_impact={s.opponent.position_x,s.opponent.position_y+1.25F,s.opponent.position_z};
    s.opponent.shot_impact={s.player.position_x-.5F,.4F,s.player.position_z-1.1F};
    s.player.velocity_z=s.opponent.velocity_z;
    s.hud.dead=false;s.hud.weapon_ready_fraction=t>=4.5 && t<5?static_cast<float>((t-4.5)*2):1;
}
}
