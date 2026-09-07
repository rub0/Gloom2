#pragma once
#include <gloom/gameplay/vertical_slice.hpp>
namespace gloom::gameplay {
class AudioPresentation {
public:
    AudioPresentation(const assets::VirtualFileSystem&,bool device);
    void scene_reset();
    void menu(double elapsed);
    void update(const SliceSnapshot&,audio::Listener,bool paused,double elapsed);
    audio::VoiceId play(audio::Cue,audio::VoiceDesc={});
    [[nodiscard]] audio::Mixer& mixer(){return mixer_;}
    [[nodiscard]] bool device_available()const{return output_->available();}
    [[nodiscard]] std::string diagnostic()const{return output_->diagnostic();}
    [[nodiscard]] std::uint64_t events_played()const{return events_played_;}
private:
    audio::Mixer mixer_;
    std::unique_ptr<audio::Output> output_;
    std::array<std::shared_ptr<const audio::Clip>,static_cast<std::size_t>(audio::Cue::count)> clips_;
    audio::EventCursor cursor_;
    std::array<audio::VoiceId,2> charge_{};
    std::array<SliceWeapon,2> charge_weapon_{};
    std::uint64_t epoch_{};
    std::array<audio::VoiceId,2> guide_{};
    std::array<audio::VoiceId,factory_pickup_count> pull_{};
    bool scene_initialized_{};std::uint32_t scene_{};std::uint64_t tick_{},local_{},events_played_{};
    void ensure_music();
    audio::VoiceId music_{};
};
}
