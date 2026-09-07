#include <gloom/audio/mixer.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace gloom::audio {
namespace {
bool finite(Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
float length(Vec3 v){return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);}
float volume(float v){if(!std::isfinite(v))throw std::invalid_argument{"Nonfinite audio volume"};return std::clamp(v,0.F,1.F);}
}
std::expected<std::shared_ptr<const Clip>,std::string> decode_clip(std::span<const std::byte> bytes){
    const auto fail=[](){return std::unexpected{std::string{"Invalid GAU1 audio: header, rate, channels, duration, length or nonfinite samples"}};};
    if(bytes.size()<16||bytes.size()>128U*1024*1024)return fail();
    auto read=[&](std::size_t offset){std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[offset+i])<<(8*i);return value;};
    if(read(0)!=0x31554147)return fail();
    const auto rate=read(4),channels=read(8),frames=read(12);
    if(rate<8000||rate>192000||(channels!=1&&channels!=2)||!frames||frames>static_cast<std::uint64_t>(rate)*1800||
       bytes.size()!=16+static_cast<std::uint64_t>(frames)*channels*4)return fail();
    auto clip=std::make_shared<Clip>();clip->rate=rate;clip->channels=channels;
    clip->samples.resize(static_cast<std::size_t>(frames)*channels);
    for(std::size_t i=0;i<clip->samples.size();++i){const float s=std::bit_cast<float>(read(16+i*4));if(!std::isfinite(s)||std::abs(s)>16)return fail();clip->samples[i]=s;}
    return clip;
}
std::expected<std::shared_ptr<const Clip>,std::string> load_clip(const assets::VirtualFileSystem& fs,std::string_view path){
    auto p=assets::VirtualPath::parse(path);if(!p)return std::unexpected{p.error()};
    auto bytes=fs.read(*p);if(!bytes)return std::unexpected{bytes.error()};
    auto result=decode_clip(*bytes);if(!result)return std::unexpected{std::string{path}+": "+result.error()};return result;
}
Mixer::Mixer(std::size_t limit):limit_{std::clamp<std::size_t>(limit,1,256)}{voices_.reserve(limit_);}
VoiceId Mixer::play(std::shared_ptr<const Clip> clip,VoiceDesc d){
    if(!clip||clip->channels<1||clip->channels>2||clip->rate<8000||clip->rate>192000||clip->samples.empty()||clip->samples.size()%clip->channels||
       !finite(d.position)||!std::isfinite(d.gain)||d.gain<0||d.gain>4||!std::isfinite(d.minimum_distance)||!std::isfinite(d.maximum_distance)||
       d.minimum_distance<=0||d.maximum_distance<=d.minimum_distance||d.bus>Bus::ambient)throw std::invalid_argument{"Invalid audio voice"};
    if(voices_.size()>=limit_){++metrics_.dropped;return 0;}
    const auto id=next_++;voices_.push_back({id,std::move(clip),d});++metrics_.started;return id;
}
void Mixer::stop(VoiceId id){std::erase_if(voices_,[&](const Voice& v){return v.id==id;});}
void Mixer::stop_scene(){std::erase_if(voices_,[](const Voice& v){return v.desc.bus!=Bus::music;});}
void Mixer::move(VoiceId id,Vec3 p){if(!finite(p))return;for(auto& v:voices_)if(v.id==id)v.desc.position=p;}
void Mixer::listener(Listener l){if(!finite(l.position)||!finite(l.forward)||!finite(l.up)||!finite(l.velocity))throw std::invalid_argument{"Invalid audio listener"};listener_=l;}
void Mixer::volumes(float master,float music,float effects){master_=volume(master);music_=volume(music);effects_=volume(effects);}
bool Mixer::playing(VoiceId id)const{return std::ranges::any_of(voices_,[&](const Voice& v){return v.id==id;});}
MixMetrics Mixer::metrics()const{auto m=metrics_;m.active=voices_.size();return m;}
std::array<float,2> Mixer::spatial_gains(const VoiceDesc& d)const{
    if(!d.spatial)return {1,1};
    const Vec3 delta{d.position.x-listener_.position.x,d.position.y-listener_.position.y,d.position.z-listener_.position.z};
    const float distance=length(delta);
    if(!std::isfinite(distance))return {0,0};
    const auto f=listener_.forward,u=listener_.up;
    const Vec3 right{f.y*u.z-f.z*u.y,f.z*u.x-f.x*u.z,f.x*u.y-f.y*u.x};
    const float divisor=distance*length(right);
    const float pan=std::isfinite(divisor)&&divisor>1e-6F?std::clamp((delta.x*right.x+delta.y*right.y+delta.z*right.z)/divisor,-1.F,1.F):0;
    const float fade=1-std::clamp((distance-d.minimum_distance)/(d.maximum_distance-d.minimum_distance),0.F,1.F);
    const float gain=d.minimum_distance/std::max(d.minimum_distance,distance)*fade*fade;
    return {gain*std::sqrt((1-pan)*.5F),gain*std::sqrt((1+pan)*.5F)};
}
void Mixer::render(std::span<float> out){
    if(out.size()%2)throw std::invalid_argument{"Stereo audio requires pairs"};
    std::ranges::fill(out,0.F);
    for(auto& v:voices_){
        const auto& c=*v.clip;const auto count=c.frames();const auto pan=spatial_gains(v.desc);
        const float gain=master_*v.desc.gain*(v.desc.bus==Bus::music?music_*(paused_?.5F:1.F):effects_*(paused_?0.F:1.F));
        for(std::size_t i=0;i<out.size()/2;++i){
            if(v.cursor>=static_cast<double>(count)){if(!v.desc.loop)break;v.cursor=std::fmod(v.cursor,static_cast<double>(count));}
            const auto a=static_cast<std::size_t>(v.cursor),b=a+1<count?a+1:(v.desc.loop?0:a);
            const float t=static_cast<float>(v.cursor-static_cast<double>(a));
            const auto sample=[&](unsigned channel){const auto k=std::min(channel,c.channels-1);return std::lerp(c.samples[a*c.channels+k],c.samples[b*c.channels+k],t);};
            float l=sample(0),r=sample(1);if(v.desc.spatial)l=r=(l+r)*.5F;
            out[i*2]+=l*gain*pan[0];out[i*2+1]+=r*gain*pan[1];v.cursor+=static_cast<double>(c.rate)/48000;
        }
    }
    const auto before=voices_.size();std::erase_if(voices_,[](const Voice& v){return !v.desc.loop&&v.cursor>=static_cast<double>(v.clip->frames());});metrics_.finished+=before-voices_.size();
    for(auto& s:out)s=std::isfinite(s)?std::clamp(s,-1.F,1.F):0.F;
}
namespace {
class NullOutput final:public Output {
    std::array<float,960> scratch_{};double remainder_{};
public:
    void pump(Mixer& mixer,double elapsed)override{if(!std::isfinite(elapsed)||elapsed<0)return;remainder_+=std::min(elapsed,.25)*48000;while(remainder_>=480){mixer.render(scratch_);remainder_-=480;}}
    bool available()const noexcept override{return false;}
    std::string diagnostic()const override{return "null audio output";}
};
}
std::unique_ptr<Output> make_null_output(){return std::make_unique<NullOutput>();}
}
