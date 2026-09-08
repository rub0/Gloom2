#pragma once
#include <gloom/gameplay/audio_presentation.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
namespace gloom::review {
inline int audio_review(const std::filesystem::path& directory,bool device){
    std::filesystem::create_directories(directory);
    assets::VirtualFileSystem fs;fs.mount("cache",std::filesystem::path{GLOOM_BINARY_ROOT}/"content");
    gameplay::AudioPresentation presentation{fs,device};
    std::ofstream log{directory/"sequence.txt"};
    log<<presentation.diagnostic()<<'\n';std::cout<<presentation.diagnostic()<<std::endl;
    if(device&&!presentation.device_available())throw std::runtime_error{"Audio review requires a real playback device"};
    const std::array cues{audio::Cue::music,audio::Cue::step,audio::Cue::jump,audio::Cue::dodge,audio::Cue::land,audio::Cue::land_grunt,
        audio::Cue::reaper_miss,audio::Cue::reaper_gore,audio::Cue::shotgun,audio::Cue::sniper,audio::Cue::minigun,audio::Cue::fireball,
        audio::Cue::electric_hit,audio::Cue::fireball_hit,audio::Cue::explosion,audio::Cue::no_ammo,audio::Cue::armor,audio::Cue::health_pack,audio::Cue::jumper,
        audio::Cue::modifier,audio::Cue::ammo,audio::Cue::shotgun_pickup,audio::Cue::change,audio::Cue::pain,audio::Cue::death,audio::Cue::spawn,
        audio::Cue::ignition,audio::Cue::lava,audio::Cue::fan,audio::Cue::atmosphere};
    constexpr unsigned seconds=58,rate=48000,frames=seconds*rate;
    std::ofstream wav{directory/"review.wav",std::ios::binary};
    const auto u16=[&](unsigned v){for(unsigned i=0;i<2;++i)wav.put(static_cast<char>((v>>(i*8))&255));};
    const auto u32=[&](unsigned v){for(unsigned i=0;i<4;++i)wav.put(static_cast<char>((v>>(i*8))&255));};
    wav.write("RIFF",4);u32(36+frames*4);wav.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(rate);u32(rate*4);u16(4);u16(16);wav.write("data",4);u32(frames*4);
    // Offline capture and real-device pass use independent mixers with identical cues.
    gameplay::AudioPresentation capture{fs,false};
    capture.menu(0);if(device)presentation.menu(0);
    const auto start=std::chrono::steady_clock::now();auto previous=start;
    std::array<float,960> samples{};
    for(unsigned block=0;block<seconds*100;++block){
        if(block%200==0){const auto i=block/200;const auto cue=cues[i];
            const float x=i%2==0?-9.F:9.F;
            audio::VoiceDesc desc{.position={x,0,-4},.gain=cue==audio::Cue::music?.12F:1.F,.spatial=i>=12};
            if(cue!=audio::Cue::music){capture.play(cue,desc);if(device)presentation.play(cue,desc);}
            log<<i*2<<" s: "<<audio::cue_paths[static_cast<std::size_t>(cue)]<<(desc.spatial?(x<0?" left":" right"):" local stereo")<<'\n';
        }
        capture.mixer().render(samples);
        for(float s:samples)u16(static_cast<std::uint16_t>(static_cast<std::int16_t>(std::clamp(s,-1.F,1.F)*32767)));
        if(device){
            const auto now=std::chrono::steady_clock::now();
            // Empty scene retains the music lifecycle while submitting queued PCM.
            presentation.menu(std::chrono::duration<double>(now-previous).count());previous=now;
            std::this_thread::sleep_until(start+std::chrono::milliseconds{static_cast<long long>(block+1)*10});
        }
    }
    log<<"Completed "<<seconds<<" seconds; device="<<presentation.device_available()<<". Human listening confirmation is separate.\n";
    std::cout<<"Audio review saved to "<<directory<<std::endl;return 0;
}
}
