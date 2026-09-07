#include <gloom/audio/mixer.hpp>
#include <iostream>
#include <stdexcept>
int main()try{
    for(unsigned i=0;i<3;++i){
        auto output=gloom::audio::make_sdl_output();
        if(output->available())throw std::runtime_error{"Invalid driver unexpectedly opened a device"};
        gloom::audio::Mixer mixer;auto clip=std::make_shared<gloom::audio::Clip>();clip->channels=1;clip->rate=48000;clip->samples.assign(480,.1F);
        mixer.play(clip);output->pump(mixer,.1);
        if(mixer.metrics().active)throw std::runtime_error{"Null fallback did not advance voices"};
        std::cout<<output->diagnostic()<<'\n';
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
