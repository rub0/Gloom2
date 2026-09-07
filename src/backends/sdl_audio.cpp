#include <gloom/audio/mixer.hpp>
#include <SDL3/SDL.h>
#include <array>

namespace gloom::audio {
namespace {
class SdlOutput final:public Output {
    SDL_AudioStream* stream_{};
    bool initialized_{};
    std::string diagnostic_;
    std::unique_ptr<Output> fallback_{make_null_output()};
public:
    SdlOutput(){
        initialized_=SDL_InitSubSystem(SDL_INIT_AUDIO);
        if(initialized_){
            const SDL_AudioSpec spec{SDL_AUDIO_F32,2,48000};
            stream_=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
            if(stream_&&!SDL_ResumeAudioStreamDevice(stream_)){SDL_DestroyAudioStream(stream_);stream_=nullptr;}
        }
        const char* name=stream_?SDL_GetAudioDeviceName(SDL_GetAudioStreamDevice(stream_)):nullptr;
        diagnostic_=stream_?std::string{"SDL3 playback: "}+(name?name:"default device"):std::string{"Audio unavailable: "}+SDL_GetError();
    }
    ~SdlOutput()override{if(stream_)SDL_DestroyAudioStream(stream_);if(initialized_)SDL_QuitSubSystem(SDL_INIT_AUDIO);}
    bool available()const noexcept override{return stream_!=nullptr;}
    std::string diagnostic()const override{return diagnostic_;}
    void pump(Mixer& mixer,double elapsed)override{
        if(!stream_){fallback_->pump(mixer,elapsed);return;}
        // Bounded 40 ms queue; SDL owns conversion to the physical device format.
        std::array<float,960> samples{};
        for(int block=0;block<4&&SDL_GetAudioStreamQueued(stream_)<48000*2*4*40/1000;++block){
            mixer.render(samples);
            if(!SDL_PutAudioStreamData(stream_,samples.data(),static_cast<int>(sizeof(samples)))){
                diagnostic_=std::string{"Audio device failed: "}+SDL_GetError();SDL_DestroyAudioStream(stream_);stream_=nullptr;break;
            }
        }
    }
};
}
std::unique_ptr<Output> make_sdl_output(){return std::make_unique<SdlOutput>();}
}
