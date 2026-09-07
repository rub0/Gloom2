#include <gloom/gameplay/audio_events.hpp>
#include <gloom/gameplay/factory_scene.hpp>
#include <gloom/gameplay/legacy_movement.hpp>
#include <cmath>
namespace gloom::gameplay {
using audio::Cue;
audio::Cue weapon_fire_cue(SliceWeapon w){
    switch(w){case SliceWeapon::shotgun:return Cue::shotgun;case SliceWeapon::sniper:return Cue::sniper;
    case SliceWeapon::minigun:return Cue::minigun;case SliceWeapon::iron_hell_goat:return Cue::fireball;default:return Cue::reaper_miss;}
}
void GameplayAudioEvents::observe(const SliceSnapshot& before,const SliceSnapshot& after,const SliceInput& first,const SliceInput& second,audio::EventJournal& journal){
    const std::array old{before.player,before.opponent},now{after.player,after.opponent};const std::array inputs{first,second};
    for(std::size_t i=0;i<2;++i){const auto& a=old[i];const auto& b=now[i];const auto& input=inputs[i];
        const audio::Vec3 p{b.position_x,b.position_y+.9F,b.position_z};
        const auto emit=[&](Cue cue){journal.emit(after.simulation_tick,b.entity,cue,p);};
        if(a.alive&&!b.alive)emit(Cue::death);
        else if(!a.alive&&b.alive)emit(Cue::spawn);
        else if(b.alive&&(b.life<a.life||b.shield<a.shield))emit(Cue::pain);
        if(a.weapon!=b.weapon&&b.alive)emit(Cue::change);
        if(a.alive&&b.alive){
            const float dx=b.position_x-a.position_x,dz=b.position_z-a.position_z;
            if(a.grounded&&b.grounded&&dx*dx+dz*dz>.000025F&&dx*dx+dz*dz<4){
                steps_[i]+=1.F/60;if(steps_[i]>=.365F){emit(Cue::step);steps_[i]-=.365F;}
            }else steps_[i]=0;
            const bool dodged=input.dodge&&(input.axis_x!=0||input.axis_z!=0)&&
                ((a.grounded&&!b.grounded)||(!a.grounded&&a.air_dodge_available&&!b.air_dodge_available));
            if(dodged)emit(Cue::dodge);
            else if(a.grounded&&!b.grounded&&input.jump&&b.velocity_y>0)emit(Cue::jump);
            if(!a.grounded&&b.grounded){const float impact=a.velocity_y+legacy_gravity/60.F;
                if(impact<-.7F*legacy_unit_scale/legacy_motion_step)emit(Cue::land);
                if(impact<-2.F*legacy_unit_scale/legacy_motion_step)emit(Cue::land_grunt);
            }
            const bool fire=input.fire_primary||input.fire_secondary;
            if(fire&&!fire_[i]&&b.weapon!=SliceWeapon::soul_reaper&&b.ammunition[static_cast<std::size_t>(b.weapon)]==0&&a.ammunition==b.ammunition)emit(Cue::no_ammo);
            fire_[i]=fire;
        }else {steps_[i]=0;fire_[i]=false;}
    }
    if(before.scene_id!=after.scene_id)return;
    for(std::size_t i=0;i<after.pickup_count&&i<before.pickup_count;++i){
        if(before.pickups[i].phase==PickupPhase::respawning||after.pickups[i].phase!=PickupPhase::respawning)continue;
        const auto& d=original_factory().pickups[i];Cue cue=Cue::modifier;
        switch(d.kind){case PickupKind::shield:cue=Cue::armor;break;
        case PickupKind::life:cue=d.reward<=5?Cue::health_vial:Cue::health_pack;break;
        case PickupKind::ammo:cue=Cue::ammo;break;
        case PickupKind::weapon:switch(d.weapon){case SliceWeapon::shotgun:cue=Cue::shotgun_pickup;break;case SliceWeapon::sniper:cue=Cue::sniper_pickup;break;case SliceWeapon::minigun:cue=Cue::minigun_pickup;break;default:cue=Cue::fireball_pickup;break;}break;
        default:break;}
        const auto p=before.pickups[i].position;journal.emit(after.simulation_tick,0,cue,{p.x,p.y,p.z},true);
    }
}
}
