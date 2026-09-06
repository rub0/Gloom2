#pragma once
#include <gloom/platform/input.hpp>

namespace gloom::platform {
// Render frames may contain no simulation tick. Keep edges until one consumes
// them, while a held key must not create another jump after landing.
struct MovementActions {
    bool jump{},dodge{},previous_jump{};
    void update(const InputState& input,bool enabled=true) noexcept {
        if(enabled){jump|=input.jump&&!previous_jump;dodge|=input.dodge;}
        else {jump=false;dodge=false;}
        previous_jump=input.jump;
    }
    void consume() noexcept {jump=false;dodge=false;}
};
}
