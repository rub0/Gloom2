#pragma once
#include <gloom/audio/mixer.hpp>
#include <algorithm>
namespace gloom::audio {
enum class Cue : std::uint8_t {
    step,jump,dodge,land,land_grunt,pain,death,spawn,no_ammo,change,
    reaper_miss,reaper_wall,reaper_wall2,reaper_gore,shotgun,sniper,minigun,fireball,
    electric_hit,ricochet,ricochet2,fireball_hit,explosion,armor,modifier,health_pack,
    health_vial,ammo,shotgun_pickup,sniper_pickup,minigun_pickup,fireball_pickup,
    music,lava,fan,atmosphere,ignition,jumper,hound_bite,hound_berserker,diamond_skin,life_dome,
    shadow_loop,shadow_in,shadow_out,shadow_flash,count
};
inline constexpr std::array<std::string_view,static_cast<std::size_t>(Cue::count)> cue_paths{
    "footsteps/step1.wav","character/jump.wav","character/sidejump.wav","land/land.wav",
    "damage/landingmalegrunt.wav","damage/pain.wav","damage/splatdeath_03.wav","gameplay/spawn.wav",
    "weapons/noammo.wav","weapons/change.wav","weapons/soulreaper/miss.wav","weapons/soulreaper/wallhit.wav",
    "weapons/soulreaper/wallhit2.wav","weapons/soulreaper/gorehit.wav","weapons/shotgun/shotgun.wav",
    "weapons/sniper/sniper.wav","weapons/minigun/shoot.wav","weapons/ironhellgoat/shootfireball2.wav",
    "weapons/hit/elec_ric.wav","weapons/hit/ric2.wav","weapons/hit/ric3.wav","weapons/hit/fireball_hit.wav",
    "weapons/explotion.wav","items/armor.wav","items/holdable.wav","items/healthpack.wav","items/healthvial.wav",
    "weapons/ammopickup.wav","weapons/shotgun/shotgunpickup.wav","weapons/sniper/sniperpickup.wav",
    "weapons/minigun/minigunpickup.wav","weapons/ironhellgoat/ironhellgoatpickup.wav","music/themegloom.wav",
    "ambient/lava_quiet.wav","ambient/bassy_fan.wav","ambient/deep_atmosphere.wav","weapons/ironhellgoat/ignite_pitch.wav","gameplay/plasma.wav",
    "character/houndbite.wav","character/houndsmell.wav","character/archangelshield.ogg","character/lifedome.mp3",
    "character/shadow.ogg","character/shadowin.ogg","character/shadowout.ogg","character/shadowflash.wav"
};
struct Event {std::uint64_t sequence{},tick{},actor{};Cue cue{};Vec3 position{};bool spatial{};};
// Factory can authoritatively consume all 73 pickups in one simulation tick.
inline constexpr std::size_t event_capacity=128;
struct EventJournal {
    std::uint64_t sequence{};std::array<Event,event_capacity> events{};
    void emit(std::uint64_t tick,std::uint64_t actor,Cue cue,Vec3 position,bool spatial=false){
        ++sequence;events[(sequence-1)%event_capacity]={sequence,tick,actor,cue,position,spatial};
    }
};
class EventCursor {
    bool initialized_{};std::uint64_t sequence_{},tick_{};
public:
    void reset(){initialized_=false;sequence_=tick_=0;}
    std::vector<Event> observe(const EventJournal& journal,std::uint64_t tick){
        std::vector<Event> result;
        if(!initialized_){initialized_=true;sequence_=journal.sequence;tick_=tick;return result;}
        if(tick<tick_||journal.sequence<sequence_)return result;
        const auto count=std::min<std::uint64_t>(journal.sequence-sequence_,event_capacity);
        for(std::uint64_t n=0;n<count;++n){const auto i=journal.sequence-(count-1-n);const auto& event=journal.events[(i-1)%event_capacity];if(event.sequence==i&&event.tick<=tick)result.push_back(event);}
        sequence_=journal.sequence;tick_=tick;return result;
    }
};
}
