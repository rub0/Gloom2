#include <gloom/gameplay/legacy_pickups.hpp>
#include <gloom/gameplay/vertical_slice.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace gloom::gameplay {
void LegacyPickups::reset(std::span<const PickupDefinition> definitions) {
    if (definitions.size()>factory_pickup_count) throw std::invalid_argument{"Too many pickups"};
    definitions_.assign(definitions.begin(),definitions.end());
    states_.clear();
    for (const auto& d:definitions_) states_.push_back({.position=d.position});
}
void LegacyPickups::tick(std::span<PickupActor> actors) {
    for (std::size_t i=0;i<states_.size();++i) {
        auto& s=states_[i]; const auto& d=definitions_[i];
        if (s.phase==PickupPhase::respawning) {
            if (s.respawn_remaining && --s.respawn_remaining) continue;
            s={.position=d.position};
        }
        if (s.phase==PickupPhase::pulling) {
            const auto owner=std::ranges::find(actors,s.pulling_player,&PickupActor::id);
            if (owner==actors.end() || !owner->alive || !owner->hold_pull ||
                owner->arsenal->active_weapon()!=SliceWeapon::soul_reaper) s={.position=d.position};
            else {
                const float x=owner->feet.x-s.position.x,y=owner->feet.y+.9F-s.position.y,z=owner->feet.z-s.position.z;
                const float distance=std::sqrt(x*x+y*y+z*z);
                // Original dynamic item's .005 * milliseconds squared, in modern units.
                const float step=std::min(distance,.005F*(1000.F/60.F)*(1000.F/60.F)*legacy_gameplay_scale);
                if(distance>0) {s.position.x+=x/distance*step;s.position.y+=y/distance*step;s.position.z+=z/distance*step;}
            }
        }
        // Stable entity order resolves simultaneous contacts; consume even at a cap.
        PickupActor* winner=nullptr;
        for (auto& actor:actors) {
            if(!actor.alive) continue;
            const float x=actor.feet.x-s.position.x,z=actor.feet.z-s.position.z;
            const float y=std::clamp(s.position.y,actor.feet.y+.45F,actor.feet.y+1.35F)-s.position.y;
            const float radius=d.radius+.45F;
            if(x*x+y*y+z*z<=radius*radius && (!winner || actor.id<winner->id)) winner=&actor;
        }
        if(!winner) continue;
        s.phase=PickupPhase::respawning;s.respawn_remaining=d.respawn_ticks;s.pulling_player=0;
        switch(d.kind) {
        case PickupKind::life:*winner->life=std::min(legacy_maximum_life,*winner->life+d.reward);break;
        case PickupKind::shield:*winner->shield=std::min(legacy_maximum_shield,*winner->shield+d.reward);break;
        case PickupKind::weapon:
            static_cast<void>(winner->arsenal->acquire(d.weapon,d.reward));
            if(winner->arsenal->active_weapon()==SliceWeapon::soul_reaper)static_cast<void>(winner->arsenal->select(d.weapon));
            break;
        case PickupKind::ammo:static_cast<void>(winner->arsenal->add_ammo(d.weapon,d.reward));break;
        case PickupKind::damage:winner->arsenal->activate_damage_modifier(d.reward);break;
        case PickupKind::cooldown:winner->arsenal->activate_cooldown_modifier(d.reward);break;
        }
    }
}
bool LegacyPickups::pull(const PickupActor& actor,physics::Vec3 direction,float range) {
    if(!actor.alive || !actor.hold_pull || actor.arsenal->active_weapon()!=SliceWeapon::soul_reaper) return false;
    for(const auto& s:states_) if(s.phase==PickupPhase::pulling && s.pulling_player==actor.id) return false;
    const float length=std::sqrt(direction.x*direction.x+direction.y*direction.y+direction.z*direction.z);
    if(!std::isfinite(length) || length<1e-5F) return false;
    direction.x/=length;direction.y/=length;direction.z/=length;
    std::size_t nearest=states_.size();float best=range;
    for(std::size_t i=0;i<states_.size();++i) {
        const auto& s=states_[i];if(s.phase!=PickupPhase::available)continue;
        const float x=s.position.x-actor.feet.x,y=s.position.y-actor.feet.y-.9F,z=s.position.z-actor.feet.z;
        const float along=x*direction.x+y*direction.y+z*direction.z;
        const float radius=definitions_[i].radius;
        const float discriminant=radius*radius-(x*x+y*y+z*z-along*along);
        if(discriminant<0 || along+radius<0)continue;
        const float entry=std::max(0.F,along-std::sqrt(discriminant));
        if(entry<best){best=entry;nearest=i;}
    }
    if(nearest==states_.size())return false;
    states_[nearest].phase=PickupPhase::pulling;states_[nearest].pulling_player=actor.id;return true;
}
}
