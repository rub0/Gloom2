#include <gloom/gameplay/first_person.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace gloom::gameplay {

FirstPersonController::FirstPersonController(const float yaw_radians,
                                             const float pitch_radians) noexcept
    : yaw_{yaw_radians},
      pitch_{std::clamp(pitch_radians, -1.3962634F, 1.3962634F)} {}

void FirstPersonController::look(const float horizontal_delta,
                                 const float vertical_delta,
                                 const float radians_per_unit) noexcept {
    if (!std::isfinite(horizontal_delta) || !std::isfinite(vertical_delta) ||
        !std::isfinite(radians_per_unit) || radians_per_unit <= 0.0F) {
        return;
    }
    yaw_ = std::remainder(yaw_ + horizontal_delta * radians_per_unit,
                          2.0F * std::numbers::pi_v<float>);
    pitch_ = std::clamp(pitch_ - vertical_delta * radians_per_unit,
                        -1.3962634F,
                        1.3962634F);
}

FirstPersonDirection FirstPersonController::forward() const noexcept {
    const float horizontal = std::cos(pitch_);
    return {.x = std::sin(yaw_) * horizontal,
            .y = std::sin(pitch_),
            .z = std::cos(yaw_) * horizontal};
}

FirstPersonMovement FirstPersonController::movement(const float local_right,
                                                     const float local_backward) const noexcept {
    const float forward_amount = -local_backward;
    const float forward_x = std::sin(yaw_);
    const float forward_z = std::cos(yaw_);
    const float right_x = std::cos(yaw_);
    const float right_z = -std::sin(yaw_);
    const float x = forward_x * forward_amount + right_x * local_right;
    const float z = forward_z * forward_amount + right_z * local_right;
    const float length = std::sqrt(x * x + z * z);
    const float normalization = length > 1.0F ? 1.0F / length : 1.0F;
    return {.world_x = x * normalization, .world_z = z * normalization};
}

float FirstPersonController::yaw() const noexcept { return yaw_; }
float FirstPersonController::pitch() const noexcept { return pitch_; }

} // namespace gloom::gameplay
