#pragma once

#include <gloom/physics/world.hpp>

#include <memory>

namespace gloom::backends {

class JoltWorld final : public physics::World {
public:
    explicit JoltWorld(physics::PhysicsSettings settings = {});
    ~JoltWorld() override;

    JoltWorld(const JoltWorld&) = delete;
    JoltWorld& operator=(const JoltWorld&) = delete;
    JoltWorld(JoltWorld&&) = delete;
    JoltWorld& operator=(JoltWorld&&) = delete;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] core::SubsystemState state() const noexcept override;

    void start() override;
    void tick(double delta_seconds) override;
    void stop() noexcept override;

    [[nodiscard]] std::uint64_t instance_generation() const noexcept override;
    void simulate(double delta_seconds) override;
    [[nodiscard]] physics::BodyId create_body(const physics::BodyDesc& description) override;
    void destroy_body(physics::BodyId body) override;
    [[nodiscard]] core::EntityId body_owner(physics::BodyId body) const override;
    [[nodiscard]] physics::Transform body_transform(physics::BodyId body) const override;
    void set_body_transform(physics::BodyId body, physics::Transform transform) override;
    void move_kinematic_body(physics::BodyId body, physics::Transform target,
                             float delta_seconds) override;
    [[nodiscard]] physics::Vec3 linear_velocity(physics::BodyId body) const override;
    void set_linear_velocity(physics::BodyId body, physics::Vec3 velocity) override;
    [[nodiscard]] std::vector<physics::TriggerEvent> take_trigger_events() override;
    [[nodiscard]] physics::CharacterId create_character(
        const physics::CharacterDesc& description) override;
    void destroy_character(physics::CharacterId character) override;
    void set_character_horizontal_velocity(physics::CharacterId character,
                                           physics::Vec3 velocity) override;
    void jump_character(physics::CharacterId character, float jump_speed) override;
    void add_character_impulse(physics::CharacterId character, physics::Vec3 impulse) override;
    void set_character_position(physics::CharacterId character, physics::Vec3 position) override;
    [[nodiscard]] physics::CharacterState character_state(
        physics::CharacterId character) const override;
    [[nodiscard]] std::vector<physics::CharacterContactEvent>
    take_character_contact_events() override;
    [[nodiscard]] physics::SimulationStats statistics() const noexcept override;
    // A fresh virtual capsule query: no hidden contact history survives rollback.
    [[nodiscard]] physics::CharacterState query_character_motion(
        const physics::CharacterDesc& description, physics::Vec3 velocity,
        float delta_seconds, physics::Vec3 gravity, float contact_padding = .02F);

private:
    struct Impl;

    void require_running() const;

    physics::PhysicsSettings settings_;
    std::unique_ptr<Impl> impl_;
    physics::SimulationStats stats_;
    core::SubsystemState state_{core::SubsystemState::stopped};
    std::uint64_t instance_generation_{0};
    double accumulator_{0.0};
};

} // namespace gloom::backends
