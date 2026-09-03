#pragma once

#include <gloom/gameplay/components.hpp>
#include <gloom/physics/components.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace gloom::gameplay {

// Deterministic ping-pong motion. All timing is expressed in simulation ticks,
// making the same phase reproducible on authority and presentation clients.
struct KinematicMotionComponent {
    physics::Vec3 start;
    physics::Vec3 end;
    std::uint32_t travel_ticks{120};
    std::uint32_t dwell_ticks{30};
    std::uint64_t phase_tick{0};
};

struct KinematicMotionSample {
    physics::Vec3 position;
    physics::Vec3 velocity;
};

[[nodiscard]] inline float kinematic_motion_alpha(const KinematicMotionComponent& motion,
                                                   const std::uint64_t tick) {
    if (motion.travel_ticks == 0) {
        throw std::invalid_argument{"Kinematic travel duration must be non-zero"};
    }
    const std::uint64_t half = static_cast<std::uint64_t>(motion.dwell_ticks) +
                               motion.travel_ticks;
    const std::uint64_t cycle = half * 2U;
    const std::uint64_t phase = tick % cycle;
    if (phase < motion.dwell_ticks) return 0.0F;
    if (phase < half) {
        return static_cast<float>(phase - motion.dwell_ticks) /
               static_cast<float>(motion.travel_ticks);
    }
    if (phase < half + motion.dwell_ticks) return 1.0F;
    return 1.0F - static_cast<float>(phase - half - motion.dwell_ticks) /
                      static_cast<float>(motion.travel_ticks);
}

[[nodiscard]] inline physics::Vec3 kinematic_motion_position(
    const KinematicMotionComponent& motion, const std::uint64_t tick) {
    const float alpha = kinematic_motion_alpha(motion, tick);
    return {motion.start.x + (motion.end.x - motion.start.x) * alpha,
            motion.start.y + (motion.end.y - motion.start.y) * alpha,
            motion.start.z + (motion.end.z - motion.start.z) * alpha};
}

[[nodiscard]] inline KinematicMotionSample advance_kinematic_motion(
    physics::World& world,
    const physics::KinematicBodyComponent& body,
    TransformComponent& transform,
    KinematicMotionComponent& motion,
    const float delta_seconds) {
    if (!std::isfinite(delta_seconds) || delta_seconds <= 0.0F) {
        throw std::invalid_argument{"Kinematic fixed delta must be finite and positive"};
    }
    const physics::Vec3 previous{transform.position_x, transform.position_y,
                                 transform.position_z};
    ++motion.phase_tick;
    const physics::Vec3 position = kinematic_motion_position(motion, motion.phase_tick);
    const physics::Vec3 velocity{(position.x - previous.x) / delta_seconds,
                                 (position.y - previous.y) / delta_seconds,
                                 (position.z - previous.z) / delta_seconds};
    transform = {.position_x = position.x,
                 .position_y = position.y,
                 .position_z = position.z,
                 .velocity_x = velocity.x,
                 .velocity_y = velocity.y,
                 .velocity_z = velocity.z};
    world.move_kinematic_body(body.body(), {.position = position}, delta_seconds);
    return {.position = position, .velocity = velocity};
}

} // namespace gloom::gameplay
