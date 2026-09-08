#include <gloom/gameplay/legacy_arsenal.hpp>
#include <gloom/gameplay/vertical_slice.hpp>
#include <gloom/gameplay/vertical_slice_network.hpp>

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace gloom::gameplay;
void require(bool value,const char* reason){if(!value)throw std::runtime_error{reason};}
void close(float a,float b,const char* reason){require(std::abs(a-b)<.001F,reason);}

int main()try{
    require(legacy_weapon_rule(SliceWeapon::soul_reaper).maximum_ammo==1,"Soul Reaper rule changed");
    require(legacy_weapon_rule(SliceWeapon::sniper).maximum_ammo==10,"Sniper cap changed");
    require(legacy_weapon_rule(SliceWeapon::shotgun).projectile_count==12,"ShotGun pellet count changed");
    require(legacy_weapon_rule(SliceWeapon::minigun).maximum_ammo==200,"MiniGun cap changed");
    require(legacy_weapon_rule(SliceWeapon::iron_hell_goat).maximum_charge_ammo==5,"IronHellGoat charge changed");
    LegacyArsenal arsenal;
    auto actions=arsenal.tick({.primary=true});
    require(actions.size()==1&&actions[0].kind==LegacyWeaponActionKind::hitscan&&actions[0].ammo_spent==0,"Soul Reaper primary differs");
    for(int i=0;i<30;++i)static_cast<void>(arsenal.tick({}));
    require(arsenal.tick({.primary=true}).size()==1,"Soul Reaper held cadence differs");
    require(arsenal.acquire(SliceWeapon::sniper,10)&&arsenal.select(SliceWeapon::sniper),"Sniper acquisition failed");
    actions=arsenal.tick({.secondary=true});
    require(actions.size()==1&&actions[0].kind==LegacyWeaponActionKind::expansive_hitscan&&arsenal.active_ammo()==8,"Sniper secondary cost differs");
    require(arsenal.acquire(SliceWeapon::shotgun,60)&&arsenal.select(SliceWeapon::shotgun),"ShotGun acquisition failed");
    actions=arsenal.tick({.primary=true});
    require(actions.size()==1&&actions[0].projectile_count==12&&arsenal.active_ammo()==59,"ShotGun primary differs");
    static_cast<void>(arsenal.tick({}));actions=arsenal.tick({.secondary=true});
    require(actions.size()==1&&actions[0].kind==LegacyWeaponActionKind::recall_projectiles,"ShotGun magnetic return missing");
    require(arsenal.acquire(SliceWeapon::minigun,200)&&arsenal.select(SliceWeapon::minigun),"MiniGun acquisition failed");
    for(int i=0;i<600;++i)static_cast<void>(arsenal.tick({.secondary=true}));
    actions=arsenal.tick({});
    require(actions.size()==1&&actions[0].kind==LegacyWeaponActionKind::charged_hitscan&&actions[0].ammo_spent==50,"MiniGun charged release differs");
    close(actions[0].charge_fraction,1,"MiniGun did not fully charge");
    require(arsenal.acquire(SliceWeapon::iron_hell_goat,30)&&arsenal.select(SliceWeapon::iron_hell_goat),"IronHellGoat acquisition failed");
    for(int i=0;i<120;++i)static_cast<void>(arsenal.tick({.primary=true}));
    actions=arsenal.tick({});
    require(actions.size()==1&&actions[0].kind==LegacyWeaponActionKind::charged_fireball&&actions[0].ammo_spent==5,"Fireball charged release differs");
    close(actions[0].damage,100,"Fireball maximum damage differs");
    close(actions[0].projectile_radius,5*.15F,"Fireball maximum radius differs");
    close(actions[0].explosion_radius,30*.15F,"Fireball explosion radius differs");
    close(actions[0].projectile_speed,.035F*1000*.15F,"Fireball millisecond speed conversion differs");
    actions=arsenal.tick({.secondary=true});
    require(actions.size()==1&&actions[0].kind==LegacyWeaponActionKind::steer_fireballs,"IronHellGoat secondary steering missing");
    require(!arsenal.add_ammo(SliceWeapon::iron_hell_goat,50)||arsenal.active_ammo()==30,"Ammo cap exceeded");
    VerticalSliceSimulation simulation{false};
    require(simulation.acquire_weapon(VerticalSliceSimulation::player_entity,SliceWeapon::shotgun,60),"Authoritative inventory acquisition failed");
    require(simulation.select_weapon(VerticalSliceSimulation::player_entity,SliceWeapon::shotgun),"Authoritative weapon switch failed");
    simulation.tick({.aim_x=1,.fire_primary=true});
    require(simulation.snapshot().projectile_count==12&&simulation.snapshot().hud.ammunition==59,"ShotGun projectiles/ammo were not authoritative");
    const auto encoded=encode_slice_snapshot(simulation.snapshot(),7,1);
    const auto decoded=decode_slice_snapshot(encoded);
    require(decoded&&decoded->projectile_count==12&&decoded->player.ammunition[2]==59,"Projectile/inventory snapshot round trip failed");
    for(int i=0;i<40;++i)simulation.tick({.aim_x=1});
    require(simulation.snapshot().opponent.life==legacy_default_life&&simulation.snapshot().player.shot_contact,
        "Blockout furnace did not occlude the authoritative magnetic projectile");
    VerticalSliceSimulation factory{{.opponent_ai_enabled=false,.original_factory=true}};
    require(factory.acquire_weapon(VerticalSliceSimulation::player_entity,SliceWeapon::iron_hell_goat,30),"Factory fireball acquisition failed");
    require(factory.select_weapon(VerticalSliceSimulation::player_entity,SliceWeapon::iron_hell_goat),"Factory fireball selection failed");
    factory.tick({.aim_z=-1,.fire_primary=true});factory.tick({.aim_z=-1});
    require(factory.snapshot().projectile_count==1,"Factory fireball was not launched");
    for(int i=0;i<599&&factory.snapshot().projectile_count;++i)factory.tick({.aim_z=-1});
    require(factory.snapshot().projectile_count==0&&factory.snapshot().player.shot_contact&&factory.snapshot().player.shot_explosion,
        "Factory fireball crossed collision or expired without exploding");
    unsigned fireball_hits=0,duplicate_explosions=0;
    for(const gloom::audio::Event& event:factory.snapshot().audio_events.events){
        fireball_hits+=event.cue==gloom::audio::Cue::fireball_hit;
        duplicate_explosions+=event.cue==gloom::audio::Cue::explosion;
    }
    require(fireball_hits==1&&duplicate_explosions==0,"Fireball impact emitted doubled explosion audio");
    const auto factory_wire=decode_slice_snapshot(encode_slice_snapshot(factory.snapshot(),8,2));
    require(factory_wire&&factory_wire->player.shot_explosion,"Projectile explosion presentation was lost on the wire");
    std::cout<<"legacy arsenal tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
