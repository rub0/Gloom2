#pragma once
#include <array>
#include <cstdint>

namespace gloom::platform {

struct DoubleTap {
    std::uint64_t previous_ms{};
    bool pending{};
    bool press(std::uint64_t now_ms,bool repeat=false) noexcept {
        if(repeat)return false;
        const bool second=pending && now_ms>=previous_ms && now_ms-previous_ms<=450;
        pending=!second;previous_ms=now_ms;return second;
    }
    void clear() noexcept {pending=false;}
};

struct InputState {
    bool move_left{false};
    bool move_right{false};
    bool move_forward{false};
    bool move_backward{false};
    bool jump{false};
    bool fire_primary{false};
    bool fire_secondary{false};
    bool use_primary_ability{false};
    bool menu_previous{false};
    bool menu_next{false};
    bool menu_confirm{false};
    float look_delta_x{0.0F};
    float look_delta_y{0.0F};
    float mouse_x{},mouse_y{};
    bool mouse_primary{},menu_back{},menu_tab{},menu_shift{},menu_up{},menu_down{};
    bool focused{true},backspace{},select_all{};
    std::array<char,256> text{};
    bool dodge{false};
    std::uint8_t weapon_selection{255};
};

struct MovementAxes {
    float x{0.0F};
    float z{0.0F};
};

[[nodiscard]] constexpr MovementAxes movement_axes(const InputState& input) noexcept {
    return {
        .x = static_cast<float>(input.move_right) - static_cast<float>(input.move_left),
        .z = static_cast<float>(input.move_backward) - static_cast<float>(input.move_forward),
    };
}

} // namespace gloom::platform
