#pragma once

#include <gloom/core/entity.hpp>
#include <gloom/core/subsystem.hpp>

#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace gloom::physics {

struct Vec3 {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};

struct TriangleMesh {
    std::vector<Vec3> vertices;
    std::vector<std::uint32_t> indices;
};

struct Quaternion {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
    float w{1.0F};
};

struct Transform {
    Vec3 position;
    Quaternion rotation;
};

struct BodyId {
    static constexpr std::uint32_t invalid_value = std::numeric_limits<std::uint32_t>::max();

    std::uint32_t value{invalid_value};

    [[nodiscard]] constexpr bool valid() const noexcept { return value != invalid_value; }
    friend constexpr bool operator==(BodyId, BodyId) = default;
};

struct CharacterId {
    static constexpr std::uint32_t invalid_value = std::numeric_limits<std::uint32_t>::max();

    std::uint32_t value{invalid_value};

    [[nodiscard]] constexpr bool valid() const noexcept { return value != invalid_value; }
    friend constexpr bool operator==(CharacterId, CharacterId) = default;
};

struct CharacterDesc {
    Vec3 position;
    float radius{0.4F};
    float cylinder_half_height{0.5F};
    float max_slope_angle_radians{0.872664626F};
    float step_up_height{0.4F};
    float step_down_height{0.5F};
    float mass{70.0F};
    float maximum_push_force{100.0F};
    core::EntityId owner;
};

struct CharacterState {
    Vec3 position;
    Vec3 velocity;
    Vec3 ground_velocity;
    Vec3 ground_normal{0.0F, 1.0F, 0.0F};
    bool grounded{false};
};

enum class CharacterContactEventType : std::uint8_t { entered, stayed, exited };

struct CharacterContactEvent {
    core::EntityId character;
    core::EntityId other;
    BodyId other_body;
    Vec3 position;
    Vec3 normal;
    std::uint64_t simulation_step{0};
    CharacterContactEventType type{CharacterContactEventType::entered};
};

enum class MotionType {
    static_body,
    dynamic,
    kinematic,
};

enum class ShapeType {
    box,
    sphere,
    triangle_mesh,
};

struct ShapeDesc {
    ShapeType type{ShapeType::box};
    Vec3 half_extent{0.5F, 0.5F, 0.5F};
    float radius{0.5F};
    std::shared_ptr<const TriangleMesh> triangle_mesh;
};

struct BodyDesc {
    ShapeDesc shape;
    Transform transform;
    MotionType motion{MotionType::static_body};
    float friction{0.2F};
    float restitution{0.0F};
    core::EntityId owner;
    bool sensor{false};
};

enum class TriggerEventType : std::uint8_t {
    entered,
    stayed,
    exited,
};

struct TriggerEvent {
    core::EntityId trigger;
    core::EntityId other;
    BodyId trigger_body;
    BodyId other_body;
    std::uint64_t simulation_step{0};
    TriggerEventType type{TriggerEventType::entered};
};

struct PhysicsSettings {
    std::uint32_t worker_threads{0};
    double fixed_time_step{1.0 / 60.0};
    std::uint32_t max_sub_steps{4};
    std::uint32_t max_bodies{65'536};
    std::uint32_t max_body_pairs{65'536};
    std::uint32_t max_contact_constraints{20'480};
    std::uint32_t temporary_allocator_bytes{10U * 1024U * 1024U};
};

struct SimulationStats {
    std::uint64_t completed_steps{0};
    std::uint32_t last_sub_steps{0};
    std::uint32_t worker_threads{0};
    double interpolation_alpha{0.0};
};

// Backends implement this interface without exposing third-party types.
class World : public core::Subsystem {
public:
    [[nodiscard]] virtual std::uint64_t instance_generation() const noexcept = 0;
    virtual void simulate(double delta_seconds) = 0;
    [[nodiscard]] virtual BodyId create_body(const BodyDesc& description) = 0;
    virtual void destroy_body(BodyId body) = 0;
    [[nodiscard]] virtual core::EntityId body_owner(BodyId body) const = 0;
    [[nodiscard]] virtual Transform body_transform(BodyId body) const = 0;
    virtual void set_body_transform(BodyId body, Transform transform) = 0;
    virtual void move_kinematic_body(BodyId body, Transform target,
                                     float delta_seconds) = 0;
    [[nodiscard]] virtual Vec3 linear_velocity(BodyId body) const = 0;
    virtual void set_linear_velocity(BodyId body, Vec3 velocity) = 0;
    [[nodiscard]] virtual float cast_ray(Vec3 origin, Vec3 direction, float maximum_distance) const = 0;
    [[nodiscard]] virtual std::vector<TriggerEvent> take_trigger_events() = 0;
    [[nodiscard]] virtual CharacterId create_character(const CharacterDesc& description) = 0;
    virtual void destroy_character(CharacterId character) = 0;
    virtual void set_character_horizontal_velocity(CharacterId character, Vec3 velocity) = 0;
    virtual void jump_character(CharacterId character, float jump_speed) = 0;
    virtual void add_character_impulse(CharacterId character, Vec3 impulse) = 0;
    virtual void set_character_position(CharacterId character, Vec3 position) = 0;
    [[nodiscard]] virtual CharacterState character_state(CharacterId character) const = 0;
    [[nodiscard]] virtual std::vector<CharacterContactEvent> take_character_contact_events() = 0;
    [[nodiscard]] virtual SimulationStats statistics() const noexcept = 0;
};

} // namespace gloom::physics
