#pragma once

namespace gloom::gameplay {

struct FirstPersonDirection {
    float x{1.0F};
    float y{0.0F};
    float z{0.0F};
};

struct FirstPersonMovement {
    float world_x{0.0F};
    float world_z{0.0F};
};

// Device-independent FPS orientation. Mouse, stick or replay input can feed
// look deltas without exposing a platform API to gameplay.
class FirstPersonController final {
public:
    explicit FirstPersonController(float yaw_radians = 1.570796327F,
                                   float pitch_radians = 0.0F) noexcept;

    void look(float horizontal_delta, float vertical_delta,
              float radians_per_unit = 0.002F) noexcept;
    [[nodiscard]] FirstPersonDirection forward() const noexcept;
    [[nodiscard]] FirstPersonMovement movement(float local_right,
                                               float local_backward) const noexcept;
    [[nodiscard]] float yaw() const noexcept;
    [[nodiscard]] float pitch() const noexcept;

private:
    float yaw_;
    float pitch_;
};

} // namespace gloom::gameplay
