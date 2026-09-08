#include <gloom/gameplay/audio_presentation.hpp>
#include <gloom/gameplay/audio_events.hpp>
#include <gloom/gameplay/vertical_slice_network.hpp>
#include <gloom/gameplay/factory_scene.hpp>
#include <gloom/gameplay/legacy_movement.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace gloom;
void require(bool value,const char* reason){if(!value)throw std::runtime_error{reason};}
bool has(const audio::EventJournal& j,audio::Cue cue){return std::ranges::any_of(j.events,[&](const auto& e){return e.sequence&&e.cue==cue;});}
int main()try{
    assets::VirtualFileSystem fs;fs.mount("game",std::filesystem::path{GLOOM_SOURCE_ROOT}/"assets");fs.mount("cache",std::filesystem::path{GLOOM_SOURCE_ROOT}/"assets/audio/cooked");
    for(const auto path:audio::cue_paths){auto loaded=audio::load_clip(fs,"cache:/"+std::string{path}+".gau");require(loaded.has_value(),"Cooked original audio failed load");}
    const auto path=assets::VirtualPath::parse("game:/audio/cooked/character/jump.wav.gau");auto bytes=*fs.read(*path);
    require(audio::decode_clip(bytes).has_value(),"game VFS audio failed");
    for(const auto offset:std::array<std::size_t,5>{0,4,8,12,16}){auto corrupt=bytes;for(std::size_t i=0;i<4;++i)corrupt[offset+i]=std::byte{0xff};require(!audio::decode_clip(corrupt),"Corrupt audio accepted");}
    bytes.pop_back();require(!audio::decode_clip(bytes),"Truncated PCM accepted");require(!audio::load_clip(fs,"cache:/../escape"),"Audio path traversal accepted");
    auto clip=std::make_shared<audio::Clip>();clip->rate=48000;clip->channels=2;clip->samples.assign(960,.5F);
    audio::Mixer mixer{2};mixer.volumes(1,1,1);auto a=mixer.play(clip,{.loop=true});auto b=mixer.play(clip);
    require(a&&b&&!mixer.play(clip),"Voice cap failed");std::array<float,960> block{};mixer.render(block);
    require(block[0]==1&&mixer.playing(a)&&!mixer.playing(b)&&mixer.metrics().dropped==1,"Mix or lifetime failed");
    mixer.stop(a);require(mixer.metrics().active==0,"Stop failed");
    const auto music=mixer.play(clip,{.bus=audio::Bus::music,.loop=true});mixer.play(clip,{.loop=true});mixer.stop_scene();
    require(mixer.playing(music)&&mixer.metrics().active==1,"Scene reset cut music");
    mixer.volumes(.5F,.4F,.8F);mixer.render(block);require(std::abs(block[0]-.1F)<1e-5F,"Bus volume failed");
    mixer.pause(true);mixer.render(block);require(std::abs(block[0]-.05F)<1e-5F,"Pause policy failed");
    auto left=mixer.spatial_gains({.position={-10,0,0},.spatial=true});auto right=mixer.spatial_gains({.position={10,0,0},.spatial=true});
    require(left[0]>left[1]&&right[1]>right[0],"Stereo perspective reversed");
    auto far=mixer.spatial_gains({.position={100,0,0},.spatial=true});require(far[1]<right[1],"Attenuation unstable");
    auto zero=mixer.spatial_gains({.spatial=true});require(std::isfinite(zero[0])&&std::abs(zero[0]-zero[1])<1e-6F,"Coincident source failed");
    auto null=audio::make_null_output();require(!null->available(),"Null backend acquired device");null->pump(mixer,.1);
    audio::EventJournal journal;audio::EventCursor cursor;journal.emit(1,1,audio::Cue::jump,{});
    require(cursor.observe(journal,1).empty(),"Late snapshot replayed history");journal.emit(2,1,audio::Cue::step,{});
    require(cursor.observe(journal,2).size()==1&&cursor.observe(journal,2).empty(),"Event duplicate");
    require(cursor.observe(journal,1).empty()&&cursor.observe(journal,2).empty(),"Reordered snapshot reset deduplication");
    cursor.reset();require(cursor.observe(journal,2).empty(),"Reconnect replayed history");
    for(unsigned i=3;i<200;++i)journal.emit(i,1,audio::Cue::step,{});
    require(cursor.observe(journal,200).size()==audio::event_capacity,"Bounded history failed");
    gameplay::SliceSnapshot before,after;before.player.entity=after.player.entity=1;before.opponent.entity=after.opponent.entity=2;
    gameplay::GameplayAudioEvents observer;audio::EventJournal events;
    for(unsigned i=0;i<23;++i){after=before;after.simulation_tick++;after.player.position_x+=.1F;observer.observe(before,after,{},{},events);before=after;}
    require(has(events,audio::Cue::step),"Moving character did not step");
    const auto steps=events.sequence;for(unsigned i=0;i<30;++i)observer.observe(after,after,{.axis_x=1},{},events);require(events.sequence==steps,"Wall input made footsteps");
    before=after;after.player.grounded=false;after.player.velocity_y=8;observer.observe(before,after,{.jump=true},{},events);require(has(events,audio::Cue::jump),"Jump missing");
    before=after;before.player.velocity_y=-20;after.player.grounded=true;after.player.velocity_y=0;observer.observe(before,after,{},{},events);
    require(has(events,audio::Cue::land)&&has(events,audio::Cue::land_grunt),"Converted landing thresholds failed");
    {gameplay::GameplayAudioEvents boundary;audio::EventJournal landing;
        auto a=before,b=after;a.player.velocity_y=-5.F;boundary.observe(a,b,{},{},landing);
        require(!has(landing,audio::Cue::land),"Soft landing emitted hard landing audio");
        a.player.velocity_y=-10;boundary.observe(a,b,{},{},landing);
        require(has(landing,audio::Cue::land)&&!has(landing,audio::Cue::land_grunt),"Landing/grunt thresholds collapsed");}
    before=after;after.player.grounded=false;observer.observe(before,after,{.axis_x=1,.dodge=true},{},events);require(has(events,audio::Cue::dodge),"Dodge missing");
    before=after;before.scene_id=after.scene_id=gameplay::original_factory().scene_id;
    before.player.position_x=after.player.position_x=gameplay::original_factory().jumper.position.x;
    before.player.position_y=after.player.position_y=gameplay::original_factory().jumper.position.y;
    before.player.position_z=after.player.position_z=gameplay::original_factory().jumper.position.z;
    before.player.velocity_y=0;after.player.velocity_y=gameplay::original_factory().jumper.force.y*gameplay::legacy_unit_scale/gameplay::legacy_motion_step;
    observer.observe(before,after,{},{},events);require(has(events,audio::Cue::jumper),"Jumper audio missing");
    before=after;after.player.life=50;observer.observe(before,after,{},{},events);require(has(events,audio::Cue::pain),"Pain missing");
    before=after;after.player.alive=false;observer.observe(before,after,{},{},events);before=after;after.player.alive=true;observer.observe(before,after,{},{},events);
    require(has(events,audio::Cue::death)&&has(events,audio::Cue::spawn),"Death/respawn missing");
    before=after;before.player.weapon=after.player.weapon=gameplay::SliceWeapon::sniper;observer.observe(before,after,{.fire_primary=true},{},events);require(has(events,audio::Cue::no_ammo),"Dry fire missing");
    for(unsigned weapon=0;weapon<gameplay::slice_weapon_count;++weapon){
        gameplay::VerticalSliceSimulation simulation{false};const auto w=static_cast<gameplay::SliceWeapon>(weapon);
        require(simulation.acquire_weapon(1,w,100),"Weapon acquisition failed");require(simulation.select_weapon(1,w),"Weapon select failed");
        for(unsigned tick=0;tick<35;++tick)simulation.tick({.aim_x=1,.fire_primary=tick<30});
        require(has(simulation.snapshot().audio_events,gameplay::weapon_fire_cue(w)),"Weapon did not emit fire audio");
        auto encoded=gameplay::encode_slice_snapshot(simulation.snapshot(),0,1);const auto decoded=gameplay::decode_slice_snapshot(encoded);
        require(decoded&&decoded->audio_events.sequence==simulation.snapshot().audio_events.sequence,"Audio network roundtrip failed");
        encoded.payload.pop_back();require(!gameplay::decode_slice_snapshot(encoded),"Truncated network audio accepted");
    }
    // Presentation tests use the same cache layout as the packaged application.
    assets::VirtualFileSystem presentation_fs;presentation_fs.mount("cache",std::filesystem::path{GLOOM_BINARY_ROOT}/"content");
    gameplay::AudioPresentation presentation{presentation_fs,false};
    gameplay::VerticalSliceSimulation factory{{.opponent_ai_enabled=false,.original_factory=true}};
    {const auto previous=factory.snapshot();auto collected=previous;
        for(std::size_t i=0;i<collected.pickup_count;++i)collected.pickups[i].phase=gameplay::PickupPhase::respawning;
        audio::EventJournal rewards;gameplay::GameplayAudioEvents pickups;pickups.observe(previous,collected,{},{},rewards);
        require(rewards.sequence==gameplay::factory_pickup_count,"Pickup audio skipped an authoritative collection");
        const auto count=rewards.sequence;pickups.observe(collected,collected,{},{},rewards);require(rewards.sequence==count,"Pickup respawn timer repeated audio");
        require(has(rewards,audio::Cue::armor)&&has(rewards,audio::Cue::ammo)&&has(rewards,audio::Cue::modifier)&&has(rewards,audio::Cue::health_pack),"Pickup kinds missing");}
    auto state=factory.snapshot();presentation.update(state,{},false,.02);require(presentation.mixer().metrics().active==10,"Factory loops missing");
    state.audio_events.emit(state.simulation_tick,1,audio::Cue::jump,{});presentation.update(state,{},false,.02);
    require(presentation.events_played()==1,"One-shot missing");presentation.update(state,{},false,.02);require(presentation.events_played()==1,"Repeated snapshot played twice");
    state.player.weapon=gameplay::SliceWeapon::iron_hell_goat;state.player.weapon_charge_fraction=.5F;presentation.update(state,{},false,.02);
    require(presentation.mixer().metrics().active>=11,"Charge loop missing");
    state.audio_epoch++;state.audio_events.emit(state.simulation_tick,1,audio::Cue::death,{});presentation.update(state,{},false,.02);
    require(presentation.events_played()==1,"Reconnect played historical one-shot");
    presentation.scene_reset();require(presentation.mixer().metrics().active==1,"Scene loop leaked or music stopped");
    std::cout<<"Audio: load/corruption, mix, voices, loop/stop, buses, spatialization, null, events, weapons, network and reconnect passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
