#include <gloom/gameplay/legacy_arsenal.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace gloom::gameplay {
namespace {
constexpr std::array rules{
    LegacyWeaponRule{SliceWeapon::soul_reaper,1,0,0,0,0,30,30,0,80,0,15,0,0,0,0},
    LegacyWeaponRule{SliceWeapon::sniper,10,1,2,0,0,90,18,0,70,30,600*legacy_gameplay_scale,0,0,0,50*legacy_gameplay_scale},
    LegacyWeaponRule{SliceWeapon::shotgun,60,1,0,0,12,90,0,0,9,5,0,12,.5F*legacy_gameplay_scale,2*legacy_gameplay_scale/.016F,0},
    LegacyWeaponRule{SliceWeapon::minigun,200,1,0,50,0,6,6,600,5,0,600*legacy_gameplay_scale,1.5F,0,0,0},
    LegacyWeaponRule{SliceWeapon::iron_hell_goat,30,0,0,5,1,45,0,120,50,100,0,0,2*legacy_gameplay_scale,.15F*60*legacy_gameplay_scale,10*legacy_gameplay_scale},
};
constexpr std::size_t index(SliceWeapon weapon) noexcept { return static_cast<std::size_t>(weapon); }
}

const LegacyWeaponRule& legacy_weapon_rule(const SliceWeapon weapon) {
    return rules.at(index(weapon));
}

const char* legacy_weapon_name(const SliceWeapon weapon) noexcept {
    constexpr std::array names{"Soul Reaper","Sniper","ShotGun","MiniGun","IronHellGoat"};
    return index(weapon)<names.size()?names[index(weapon)]:"Invalid";
}

LegacyArsenal::LegacyArsenal() { reset(); }

void LegacyArsenal::reset(const SliceWeapon initial_weapon) noexcept {
    clear_modifiers();
    owned_.fill(false); ammo_.fill(0); active_=initial_weapon;
    if(index(initial_weapon)>=slice_weapon_count) active_=SliceWeapon::soul_reaper;
    owned_[index(active_)]=true; ammo_[index(active_)]=legacy_weapon_rule(active_).maximum_ammo;
    cooldown_=charge_ticks_=charge_ammo_=0; previous_primary_=previous_secondary_=false;
    magnetic_projectiles_=false;controllable_fireballs_=false;minigun_dispersion_=1.5F; action_count_=0;
}

bool LegacyArsenal::acquire(const SliceWeapon weapon,const std::uint16_t ammo) noexcept {
    if(index(weapon)>=slice_weapon_count)return false;
    owned_[index(weapon)]=true; static_cast<void>(add_ammo(weapon,ammo)); return true;
}
bool LegacyArsenal::add_ammo(const SliceWeapon weapon,const std::uint16_t amount) noexcept {
    if(index(weapon)>=slice_weapon_count)return false;
    auto& value=ammo_[index(weapon)];const auto old=value;
    value=static_cast<std::uint16_t>(std::min<unsigned>(legacy_weapon_rule(weapon).maximum_ammo,value+amount));
    return value!=old;
}
bool LegacyArsenal::select(const SliceWeapon weapon) noexcept {
    if(index(weapon)>=slice_weapon_count || !owned_[index(weapon)])return false;
    if(active_==weapon)return true;
    active_=weapon;cooldown_=charge_ticks_=charge_ammo_=0;previous_primary_=previous_secondary_=false;return true;
}
SliceWeapon LegacyArsenal::active_weapon()const noexcept{return active_;}
bool LegacyArsenal::owns(const SliceWeapon weapon)const noexcept{return index(weapon)<slice_weapon_count&&owned_[index(weapon)];}
std::uint16_t LegacyArsenal::ammo(const SliceWeapon weapon)const noexcept{return index(weapon)<slice_weapon_count?ammo_[index(weapon)]:0;}
std::uint16_t LegacyArsenal::active_ammo()const noexcept{return ammo(active_);}
std::uint16_t LegacyArsenal::cooldown_remaining()const noexcept{return cooldown_;}
float LegacyArsenal::charge_fraction()const noexcept {const auto max=legacy_weapon_rule(active_).charge_ticks;return max?std::min(1.F,static_cast<float>(charge_ticks_)/max):0;}
void LegacyArsenal::emit(LegacyWeaponAction action)noexcept{if(action_count_<actions_.size())actions_[action_count_++]=action;}
bool LegacyArsenal::spend(const std::uint16_t amount)noexcept{auto& a=ammo_[index(active_)];if(a<amount)return false;a=static_cast<std::uint16_t>(a-amount);return true;}

std::span<const LegacyWeaponAction> LegacyArsenal::tick(const LegacyArsenalInput input) noexcept {
    action_count_=0;if(cooldown_)--cooldown_;auto r=legacy_weapon_rule(active_);
    const float damage_scale=damage_ticks_?1.F+damage_percent_*.01F:1.F;
    if(modifier_ticks_) {
        const auto reduced=[&](std::uint16_t value){return value?static_cast<std::uint16_t>(std::max(1.F,std::ceil(value*(1.F-cooldown_percent_*.01F)))):std::uint16_t{0};};
        r.primary_cooldown_ticks=reduced(r.primary_cooldown_ticks);
        r.secondary_cooldown_ticks=reduced(r.secondary_cooldown_ticks);
    }
    const bool primary_edge=input.primary&&!previous_primary_;const bool primary_release=!input.primary&&previous_primary_;
    const bool secondary_edge=input.secondary&&!previous_secondary_;const bool secondary_release=!input.secondary&&previous_secondary_;
    const auto basic=[&](LegacyWeaponActionKind kind,float damage,float range,std::uint16_t count,std::uint16_t spent){emit({kind,active_,count,spent,damage,range,r.dispersion_degrees,r.projectile_radius,r.projectile_speed,r.explosion_radius,0});};
    switch(active_){
    case SliceWeapon::soul_reaper:
        if(input.primary&&!input.secondary&&!cooldown_){cooldown_=r.primary_cooldown_ticks;basic(LegacyWeaponActionKind::hitscan,r.primary_damage,r.range,0,0);}
        if(secondary_edge)basic(LegacyWeaponActionKind::pull_item,0,250*legacy_gameplay_scale,0,0);
        break;
    case SliceWeapon::sniper:
        if(primary_edge&&!cooldown_&&spend(r.primary_ammo)){cooldown_=r.primary_cooldown_ticks;basic(LegacyWeaponActionKind::hitscan,r.primary_damage,r.range,0,r.primary_ammo);}
        if(secondary_edge&&!cooldown_&&spend(r.secondary_ammo)){cooldown_=r.secondary_cooldown_ticks;basic(LegacyWeaponActionKind::expansive_hitscan,r.secondary_damage,r.range,0,r.secondary_ammo);}
        break;
    case SliceWeapon::shotgun:
        if(primary_edge&&!cooldown_&&spend(r.primary_ammo)){cooldown_=r.primary_cooldown_ticks;magnetic_projectiles_=true;basic(LegacyWeaponActionKind::magnetic_projectiles,r.primary_damage,0,r.projectile_count,r.primary_ammo);}
        if(secondary_edge&&magnetic_projectiles_){magnetic_projectiles_=false;basic(LegacyWeaponActionKind::recall_projectiles,r.secondary_damage,0,r.projectile_count,0);}
        break;
    case SliceWeapon::minigun:
        if(input.primary&&!input.secondary&&!cooldown_&&spend(1)){cooldown_=r.primary_cooldown_ticks;emit({LegacyWeaponActionKind::hitscan,active_,0,1,r.primary_damage,r.range,minigun_dispersion_,0,0,0,0});minigun_dispersion_=std::max(0.F,minigun_dispersion_-.2F);}
        if(!input.primary)minigun_dispersion_=r.dispersion_degrees;
        if(input.secondary&&!cooldown_&&!charge_ticks_&&!charge_ammo_&&spend(1))++charge_ammo_;
        if(input.secondary&&(charge_ticks_||!cooldown_)&&ammo_[index(active_)]&&charge_ticks_<r.charge_ticks){++charge_ticks_;if(charge_ticks_%12==0&&charge_ammo_<r.maximum_charge_ammo&&spend(1))++charge_ammo_;}
        if(secondary_release&&charge_ammo_){const float f=static_cast<float>(charge_ammo_)/r.maximum_charge_ammo;emit({LegacyWeaponActionKind::charged_hitscan,active_,0,charge_ammo_,r.primary_damage*charge_ammo_,r.range,r.dispersion_degrees,0,0,0,f});charge_ticks_=charge_ammo_=0;cooldown_=r.secondary_cooldown_ticks;}
        break;
    case SliceWeapon::iron_hell_goat:
        if(input.primary&&(charge_ticks_||!cooldown_)&&ammo_[index(active_)]&&charge_ticks_<r.charge_ticks){++charge_ticks_;const auto target=static_cast<std::uint16_t>(std::max(1.F,std::ceil(charge_fraction()*r.maximum_charge_ammo)));while(charge_ammo_<target&&spend(1))++charge_ammo_;}
        if(primary_release&&charge_ammo_){const float f=charge_fraction();emit({LegacyWeaponActionKind::charged_fireball,active_,1,charge_ammo_,r.primary_damage+(r.secondary_damage-r.primary_damage)*f,0,0,(2+3*f)*legacy_gameplay_scale,(.15F+(.035F-.15F)*f)*60*legacy_gameplay_scale,(10+20*f)*legacy_gameplay_scale,f});controllable_fireballs_=true;charge_ticks_=charge_ammo_=0;cooldown_=r.primary_cooldown_ticks;}
        if(input.secondary&&controllable_fireballs_)basic(LegacyWeaponActionKind::steer_fireballs,0,0,0,0);
        break;
    }
    for(std::size_t i=0;i<action_count_;++i)actions_[i].damage*=damage_scale;
    if(damage_ticks_)--damage_ticks_;
    if(modifier_ticks_)--modifier_ticks_;
    previous_primary_=input.primary;previous_secondary_=input.secondary;return {actions_.data(),action_count_};
}
} // namespace gloom::gameplay
