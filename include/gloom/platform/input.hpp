#pragma once

namespace gloom::platform {

struct InputState {
    bool move_left{false};
    bool move_right{false};
    bool move_forward{false};
    bool move_backward{false};
    bool jump{false};
    bool fire_primary{false};
    bool use_primary_ability{false};
    bool menu_previous{false};
    bool menu_next{false};
    bool menu_confirm{false};
    float look_delta_x{0.0F};
    float look_delta_y{0.0F};
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
