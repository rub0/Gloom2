#pragma once

#include <gloom/core/entity.hpp>
#include <gloom/physics/world.hpp>

#include <stdexcept>
#include <utility>

namespace gloom::physics {

// Move-only ownership component for one backend body. The template arguments
// make static, dynamic, kinematic and trigger storage distinct component types
// while keeping their lifetime rules identical.
template <MotionType Motion, bool Sensor>
class BasicBodyComponent final {
public:
    BasicBodyComponent(World& world, const core::EntityId entity, BodyDesc description)
        : world_{&world}, entity_{entity} {
        if (!entity.valid()) {
            throw std::invalid_argument{"Physics component requires a valid logical entity"};
        }
        description.motion = Motion;
        description.sensor = Sensor;
        description.owner = entity;
        body_ = world_->create_body(description);
        world_generation_ = world_->instance_generation();
    }

    ~BasicBodyComponent() { release(); }

    BasicBodyComponent(const BasicBodyComponent&) = delete;
    BasicBodyComponent& operator=(const BasicBodyComponent&) = delete;

    BasicBodyComponent(BasicBodyComponent&& other) noexcept
        : world_{std::exchange(other.world_, nullptr)},
          entity_{std::exchange(other.entity_, {})},
          body_{std::exchange(other.body_, {})},
          world_generation_{std::exchange(other.world_generation_, 0)} {}

    BasicBodyComponent& operator=(BasicBodyComponent&& other) noexcept {
        if (this != &other) {
            release();
            world_ = std::exchange(other.world_, nullptr);
            entity_ = std::exchange(other.entity_, {});
            body_ = std::exchange(other.body_, {});
            world_generation_ = std::exchange(other.world_generation_, 0);
        }
        return *this;
    }

    [[nodiscard]] core::EntityId entity() const noexcept { return entity_; }
    [[nodiscard]] BodyId body() const noexcept { return body_; }
    [[nodiscard]] static constexpr MotionType motion_type() noexcept { return Motion; }
    [[nodiscard]] static constexpr bool is_trigger() noexcept { return Sensor; }

    void reset() {
        if (world_ != nullptr && body_.valid() &&
            world_->state() == core::SubsystemState::running &&
            world_->instance_generation() == world_generation_) {
            world_->destroy_body(body_);
        }
        world_ = nullptr;
        entity_ = {};
        body_ = {};
        world_generation_ = 0;
    }

private:
    void release() noexcept {
        try {
            reset();
        } catch (...) {
            // Component destruction cannot throw. Explicit reset() remains
            // available when the caller needs to observe backend failures.
            world_ = nullptr;
            entity_ = {};
            body_ = {};
            world_generation_ = 0;
        }
    }

    World* world_{nullptr};
    core::EntityId entity_;
    BodyId body_;
    std::uint64_t world_generation_{0};
};

using StaticBodyComponent = BasicBodyComponent<MotionType::static_body, false>;
using DynamicBodyComponent = BasicBodyComponent<MotionType::dynamic, false>;
using KinematicBodyComponent = BasicBodyComponent<MotionType::kinematic, false>;
using TriggerComponent = BasicBodyComponent<MotionType::static_body, true>;

} // namespace gloom::physics
