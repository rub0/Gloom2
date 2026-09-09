#include <gloom/gameplay/audio_presentation.hpp>
#include <gloom/gameplay/audio_events.hpp>
#include <stdexcept>
#include <cstdlib>
#include <cmath>
namespace gloom::gameplay {
AudioPresentation::AudioPresentation(const assets::VirtualFileSystem& fs,bool device):output_{device?audio::make_sdl_output():audio::make_null_output()}{
    const auto volume=[](const char* key,float fallback){
#ifdef _MSC_VER
        char* buffer=nullptr;std::size_t size=0;
        if(_dupenv_s(&buffer,&size,key)!=0)throw std::runtime_error{"Cannot read audio volume setting"};
        const std::unique_ptr<char,decltype(&std::free)> owned{buffer,&std::free};const auto* value=owned.get();
#else
        const auto* value=std::getenv(key);
#endif
        if(!value)return fallback;
        std::size_t end{};const float result=std::stof(value,&end);
        if(end!=std::string_view{value}.size()||!std::isfinite(result)||result<0||result>1)throw std::invalid_argument{std::string{key}+" must be in [0,1]"};return result;};
    mixer_.volumes(volume("GLOOM_AUDIO_MASTER",.7F),volume("GLOOM_AUDIO_MUSIC",.35F),volume("GLOOM_AUDIO_EFFECTS",.8F));
    for(std::size_t i=0;i<clips_.size();++i){
        auto clip=audio::load_clip(fs,"cache:/audio/"+std::string{audio::cue_paths[i]}+".gau");
        if(!clip)throw std::runtime_error{clip.error()};clips_[i]=*clip;
    }
}
audio::VoiceId AudioPresentation::play(audio::Cue cue,audio::VoiceDesc desc){return mixer_.play(clips_.at(static_cast<std::size_t>(cue)),desc);}
void AudioPresentation::ensure_music(){if(!mixer_.playing(music_))music_=play(audio::Cue::music,{.bus=audio::Bus::music,.gain=.6F,.loop=true});}
void AudioPresentation::scene_reset(){
    mixer_.stop_scene();mixer_.pause(false);cursor_.reset();charge_={};guide_={};shadow_={};pull_={};
    scene_initialized_=false;local_=tick_=0;
}
void AudioPresentation::menu(double elapsed){ensure_music();mixer_.pause(false);output_->pump(mixer_,elapsed);}
void AudioPresentation::update(const SliceSnapshot& s,audio::Listener listener,bool paused,double elapsed){
    if(scene_initialized_&&(s.scene_id!=scene_||s.player.entity!=local_||s.audio_epoch!=epoch_))scene_reset();
    if(scene_initialized_&&s.simulation_tick<tick_){output_->pump(mixer_,elapsed);return;}
    epoch_=s.audio_epoch;
    ensure_music();mixer_.listener(listener);mixer_.pause(paused);
    if(!scene_initialized_){scene_initialized_=true;scene_=s.scene_id;local_=s.player.entity;
        // Factory_client.txt positions multiplied by the same .15 map scale.
        if(s.scene_id){
            constexpr std::array<audio::Vec3,9> positions{{{-6.34871F,1.65F,-71.602F},{-123.08F,1.65F,-64.36F},{-225.305F,1.65F,-60.94F},{-324.381F,1.65F,-56.35F},{192.972F,88.67F,-67.08F},{121.414F,3.0115F,-230.99F},{-51.9089F,47.9726F,-241.786F},{-400.232F,74.89F,-73.5654F},{-34.8509F,78.4421F,58.429F}}};
            for(std::size_t i=0;i<positions.size();++i){const auto p=positions[i];play(i<4?audio::Cue::lava:i==4?audio::Cue::fan:audio::Cue::atmosphere,
                {.bus=audio::Bus::ambient,.position={p.x*.15F,p.y*.15F,p.z*.15F},.gain=.25F,.spatial=true,.loop=true});}
        }
    }
    for(const auto& e:cursor_.observe(s.audio_events,s.simulation_tick)){
        // Paused events are consumed, so unpausing never bursts old combat sounds.
        if(!paused&&!(e.cue==audio::Cue::spree&&e.actor!=s.player.entity)){play(e.cue,{.position=e.position,.spatial=e.spatial||e.actor!=s.player.entity});++events_played_;}
    }
    const std::array combatants{s.player,s.opponent};
    for(std::size_t i=0;i<2;++i){const auto& c=combatants[i];const bool active=c.alive&&c.weapon_charge_fraction>0;
        if(!active||charge_weapon_[i]!=c.weapon){mixer_.stop(charge_[i]);charge_[i]=0;}
        if(active&&!mixer_.playing(charge_[i])){charge_[i]=play(audio::Cue::ignition,
            {.position={c.position_x,c.position_y+.9F,c.position_z},.gain=.12F,.spatial=i!=0,.loop=true});charge_weapon_[i]=c.weapon;}
        if(active)mixer_.move(charge_[i],{c.position_x,c.position_y+.9F,c.position_z});
        if(!c.alive||!c.audio_guiding){mixer_.stop(guide_[i]);guide_[i]=0;}
        else {if(!mixer_.playing(guide_[i]))guide_[i]=play(audio::Cue::ignition,{.gain=.08F,.spatial=i!=0,.loop=true});mixer_.move(guide_[i],{c.position_x,c.position_y+.9F,c.position_z});}
        const bool invisible=c.alive&&c.ability==SliceAbility::invisibility&&c.primary_ability_active;
        if(!invisible){mixer_.stop(shadow_[i]);shadow_[i]=0;}
        else {
            if(!mixer_.playing(shadow_[i]))shadow_[i]=play(audio::Cue::shadow_loop,{.gain=.32F,.spatial=i!=0,.loop=true});
            mixer_.move(shadow_[i],{c.position_x,c.position_y+.9F,c.position_z});
        }
    }
    for(std::size_t i=0;i<pull_.size();++i){
        if(i>=s.pickup_count||s.pickups[i].phase!=PickupPhase::pulling){mixer_.stop(pull_[i]);pull_[i]=0;}
        else {if(!mixer_.playing(pull_[i]))pull_[i]=play(audio::Cue::electric_hit,{.gain=.08F,.spatial=true,.loop=true});const auto p=s.pickups[i].position;mixer_.move(pull_[i],{p.x,p.y,p.z});}
    }
    tick_=s.simulation_tick;output_->pump(mixer_,elapsed);
}
}
