#pragma once
#include <gloom/gameplay/vertical_slice.hpp>
namespace gloom::gameplay {
class GameplayAudioEvents {
public:
    void observe(const SliceSnapshot& before,const SliceSnapshot& after,const SliceInput&,const SliceInput&,audio::EventJournal&);
private:
    std::array<float,2> steps_{};std::array<bool,2> fire_{};
};
[[nodiscard]] audio::Cue weapon_fire_cue(SliceWeapon);
}
