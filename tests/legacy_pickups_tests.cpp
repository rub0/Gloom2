#include <gloom/gameplay/factory_scene.hpp>
#include <gloom/gameplay/vertical_slice_network.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

using namespace gloom::gameplay;
void require(bool value,const char* reason){if(!value)throw std::runtime_error{reason};}
int main()try {
    const auto& map=original_factory();
    require(map.pickups.size()==73,"Factory must contain all 73 pickups");
    std::set<std::string> names;
    std::set<std::pair<PickupKind,SliceWeapon>> types;
    for(const auto& d:map.pickups) {
        require(names.insert(d.name).second,"Duplicate pickup identity");types.emplace(d.kind,d.weapon);
        LegacyPickups pickups;pickups.reset(std::span{&d,1});
        LegacyArsenal a,b;float life_a=240,life_b=120,shield_a=145,shield_b=0;
        std::array actors{PickupActor{2,d.position,true,false,&b,&life_b,&shield_b},
                          PickupActor{1,d.position,true,false,&a,&life_a,&shield_a}};
        pickups.tick(actors);
        require(pickups.views()[0].phase==PickupPhase::respawning,"Contact failed");
        require(pickups.views()[0].respawn_remaining==d.respawn_ticks,"Respawn rule differs");
        require(life_b==120 && shield_b==0 && b.active_weapon()==SliceWeapon::soul_reaper &&
            b.ammo(SliceWeapon::sniper)==0 && b.ammo(SliceWeapon::shotgun)==0 &&
            b.ammo(SliceWeapon::minigun)==0 && b.ammo(SliceWeapon::iron_hell_goat)==0 &&
            b.damage_modifier_ticks()==0 && b.cooldown_modifier_ticks()==0,"Contested pickup rewarded both actors");
        switch(d.kind) {
        case PickupKind::life:require(life_a==std::min(250.F,240.F+d.reward),"Life reward/cap differs");break;
        case PickupKind::shield:require(shield_a==150,"Shield cap differs");break;
        case PickupKind::weapon:require(a.owns(d.weapon)&&a.active_weapon()==d.weapon,"Weapon acquisition/autoselection missing");break;
        case PickupKind::ammo:require(!a.owns(d.weapon)&&a.ammo(d.weapon)==d.reward,"Unowned ammunition reserve differs");break;
        case PickupKind::damage:require(a.damage_modifier_ticks()==900,"Damage duration differs");break;
        case PickupKind::cooldown:require(a.cooldown_modifier_ticks()==900,"Cooldown duration differs");break;
        }
        actors[0].alive=actors[1].alive=false;
        for(unsigned i=1;i<d.respawn_ticks;++i)pickups.tick(actors);
        require(pickups.views()[0].phase==PickupPhase::respawning,"Early respawn");
        pickups.tick(actors);require(pickups.views()[0].phase==PickupPhase::available,"Missing respawn");
    }
    require(types.size()==12,"Pickup reward families changed");
    LegacyArsenal reserve;
    require(reserve.add_ammo(SliceWeapon::sniper,65535) && reserve.ammo(SliceWeapon::sniper)==10 && !reserve.owns(SliceWeapon::sniper),"Unowned ammo cap/ownership differs");
    require(reserve.acquire(SliceWeapon::sniper,10) && reserve.ammo(SliceWeapon::sniper)==10,"Acquisition exceeded reserve cap");
    // Independent 15-second modifiers, refresh without exponential stacking.
    LegacyArsenal arsenal;arsenal.activate_damage_modifier(200);arsenal.activate_cooldown_modifier(200);
    auto actions=arsenal.tick({.primary=true});
    require(actions.size()==1 && actions[0].damage==240 && arsenal.cooldown_remaining()==1,"Extreme modifiers are unsafe");
    arsenal.activate_damage_modifier(200);
    for(unsigned i=0;i<899;++i) {
        actions=arsenal.tick({.primary=true});require(actions.size()==1&&actions[0].damage==240,"Modifier stacking/cadence differs");
    }
    require(arsenal.cooldown_modifier_ticks()==0 && arsenal.damage_modifier_ticks()==1,"Modifier timers crossed");
    actions=arsenal.tick({.primary=true});require(actions[0].damage==240&&arsenal.cooldown_remaining()==30,"Cooldown did not restore");
    for(unsigned i=0;i<30;++i)actions=arsenal.tick({.primary=true});
    require(actions.size()==1&&actions[0].damage==80,"Damage did not restore");
    // Pulling reserves one identity, supports cancellation and grants on contact only.
    PickupDefinition d{.name="pull",.kind=PickupKind::life,.position={5,.9F,0},.reward=17,.respawn_ticks=1500};
    LegacyPickups pickups;pickups.reset(std::span{&d,1});
    LegacyArsenal a,b;float life=120,other_life=120,shield=0,other_shield=0;
    std::array actors{PickupActor{1,{0,0,0},true,true,&a,&life,&shield},PickupActor{2,{0,0,0},true,true,&b,&other_life,&other_shield}};
    require(!pickups.pull(actors[0],{0,0,0},37.5F),"Zero aim accepted");
    require(pickups.pull(actors[0],{1,0,0},37.5F)&&!pickups.pull(actors[1],{1,0,0},37.5F),"Duplicate pull reservation");
    pickups.tick(actors);require(life==120&&pickups.views()[0].position.x<5,"Pull rewarded before contact");
    actors[0].hold_pull=false;pickups.tick(actors);
    require(pickups.views()[0].phase==PickupPhase::available&&pickups.views()[0].position.x==5,"Release failed to restore spawn");
    actors[0].hold_pull=true;require(pickups.pull(actors[0],{1,0,0},37.5F),"Pull could not restart");
    for(unsigned i=0;i<30;++i)pickups.tick(actors);
    require(life==137&&other_life==120&&pickups.views()[0].phase==PickupPhase::respawning,"Pull duplicated/lost reward");
    // Full-state wire representation, including invalid/malicious fields.
    VerticalSliceSimulation simulation{VerticalSliceSettings{.opponent_ai_enabled=false,.original_factory=true}};
    auto snapshot=simulation.snapshot();require(snapshot.pickup_count==73,"Simulation omitted pickups");
    snapshot.pickups[0].phase=PickupPhase::respawning;snapshot.pickups[0].respawn_remaining=7000;
    snapshot.pickups[1].phase=PickupPhase::pulling;snapshot.pickups[1].pulling_player=2;
    snapshot.player.damage_modifier_ticks=900;
    const auto wire=encode_slice_snapshot(snapshot,0,1);
    const auto decoded=decode_slice_snapshot(wire);
    require(decoded&&decoded->pickups[0].respawn_remaining==7000&&decoded->pickups[1].pulling_player==2&&decoded->player.damage_modifier_ticks==900,"Pickup snapshot round trip differs");
    auto invalid=wire;invalid.payload.back()=std::byte{3};
    require(!decode_slice_snapshot(invalid),"Invalid pulling owner accepted");
    invalid=wire;invalid.payload.pop_back();require(!decode_slice_snapshot(invalid),"Truncated pickup snapshot accepted");
    snapshot.player.ammunition[1]=11;
    bool rejected=false;try{static_cast<void>(encode_slice_snapshot(snapshot,0,2));}catch(const std::invalid_argument&){rejected=true;}
    require(rejected,"Over-cap ammunition accepted on wire");
    std::cout<<"73 pickup locations, rewards, contention, timers, pull and protocol passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
