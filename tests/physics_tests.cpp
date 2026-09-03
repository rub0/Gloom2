#include <gloom/backends/jolt_world.hpp>
#include <gloom/core/entity.hpp>
#include <gloom/physics/components.hpp>

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

void expect_near(const float actual, const float expected, const float tolerance, const char* message) {
    if (std::abs(actual - expected) > tolerance) {
        throw std::runtime_error{message};
    }
}

template <typename Function>
void expect_throws(Function&& function, const char* message) {
    try {
        function();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error{message};
}

} // namespace

int main() try {
    gloom::backends::JoltWorld world{gloom::physics::PhysicsSettings{
        .worker_threads = 2,
        .fixed_time_step = 1.0 / 60.0,
        .max_sub_steps = 4,
        .max_bodies = 128,
        .max_body_pairs = 1'024,
        .max_contact_constraints = 1'024,
        .temporary_allocator_bytes = 2U * 1024U * 1024U,
    }};
    world.start();

    const auto floor = world.create_body(gloom::physics::BodyDesc{
        .shape = {.type = gloom::physics::ShapeType::box, .half_extent = {10.0F, 0.5F, 10.0F}},
        .transform = {.position = {0.0F, -0.5F, 0.0F}},
        .motion = gloom::physics::MotionType::static_body,
    });
    const auto sphere = world.create_body(gloom::physics::BodyDesc{
        .shape = {.type = gloom::physics::ShapeType::sphere, .radius = 0.5F},
        .transform = {.position = {0.0F, 5.0F, 0.0F}},
        .motion = gloom::physics::MotionType::dynamic,
        .friction = 0.5F,
    });
    const auto character = world.create_character(gloom::physics::CharacterDesc{
        .position = {0.0F, 0.05F, -2.0F},
        .radius = 0.4F,
        .cylinder_half_height = 0.5F,
    });

    world.simulate(1.0 / 120.0);
    expect(world.statistics().completed_steps == 0, "A partial fixed step was simulated too early");
    expect_near(static_cast<float>(world.statistics().interpolation_alpha),
                0.5F,
                0.001F,
                "Fixed-step interpolation alpha is incorrect");
    world.simulate(1.0 / 120.0);
    expect(world.statistics().completed_steps == 1, "Two half steps did not produce one fixed step");

    for (int step = 1; step < 180; ++step) {
        world.simulate(1.0 / 60.0);
    }

    const auto settled = world.body_transform(sphere);
    expect(settled.position.y < 1.0F, "Dynamic sphere did not fall under gravity");
    expect_near(settled.position.y, 0.5F, 0.08F, "Dynamic sphere did not settle on the floor");
    expect(world.statistics().completed_steps == 180, "Physics fixed-step count is incorrect");
    expect(world.statistics().worker_threads == 2, "Requested Jolt worker count was not used");

    const auto grounded_character = world.character_state(character);
    expect(grounded_character.grounded, "Virtual character did not detect the floor");
    expect_near(grounded_character.position.y,
                0.0F,
                0.08F,
                "Virtual character did not settle at floor height");
    world.set_character_horizontal_velocity(character, {2.0F, 0.0F, 0.0F});
    for (int step = 0; step < 60; ++step) {
        world.simulate(1.0 / 60.0);
    }
    const auto moved_character = world.character_state(character);
    expect_near(moved_character.position.x,
                2.0F,
                0.08F,
                "Virtual character did not follow horizontal velocity");
    expect(moved_character.grounded, "Moving virtual character lost floor support");

    world.jump_character(character, 5.5F);
    world.simulate(1.0 / 60.0);
    const auto jumping_character = world.character_state(character);
    expect(!jumping_character.grounded && jumping_character.position.y > 0.0F &&
               jumping_character.velocity.y > 0.0F,
           "Virtual character did not apply its jump impulse");

    world.set_linear_velocity(sphere, {1.0F, 0.0F, 0.0F});
    expect_near(world.linear_velocity(sphere).x, 1.0F, 0.001F, "Body velocity round trip failed");

    gloom::core::EntityRegistry entities;
    const auto trigger_entity = entities.create();
    const auto visitor_entity = entities.create();
    const auto static_entity = entities.create();
    const auto kinematic_entity = entities.create();
    auto& trigger = entities.emplace<gloom::physics::TriggerComponent>(
        trigger_entity,
        world,
        trigger_entity,
        gloom::physics::BodyDesc{
            .shape = {.type = gloom::physics::ShapeType::box,
                      .half_extent = {0.5F, 3.0F, 0.5F}},
            .transform = {.position = {0.0F, 2.5F, 3.0F}},
        });
    auto& visitor = entities.emplace<gloom::physics::DynamicBodyComponent>(
        visitor_entity,
        world,
        visitor_entity,
        gloom::physics::BodyDesc{
            .shape = {.type = gloom::physics::ShapeType::sphere, .radius = 0.25F},
            .transform = {.position = {-2.0F, 2.0F, 3.0F}},
            .friction = 0.0F,
        });
    auto& static_component = entities.emplace<gloom::physics::StaticBodyComponent>(
        static_entity,
        world,
        static_entity,
        gloom::physics::BodyDesc{
            .shape = {.type = gloom::physics::ShapeType::box,
                      .half_extent = {0.25F, 0.25F, 0.25F}},
            .transform = {.position = {8.0F, 0.25F, 8.0F}},
        });
    auto& kinematic_component = entities.emplace<gloom::physics::KinematicBodyComponent>(
        kinematic_entity,
        world,
        kinematic_entity,
        gloom::physics::BodyDesc{
            .shape = {.type = gloom::physics::ShapeType::box,
                      .half_extent = {0.25F, 0.25F, 0.25F}},
            .transform = {.position = {-8.0F, 0.25F, -8.0F}},
        });
    expect(trigger.is_trigger() && !visitor.is_trigger() &&
               static_component.motion_type() == gloom::physics::MotionType::static_body &&
               visitor.motion_type() == gloom::physics::MotionType::dynamic &&
               kinematic_component.motion_type() == gloom::physics::MotionType::kinematic,
           "Logical physics components did not preserve their body roles");
    world.move_kinematic_body(
        kinematic_component.body(), {.position = {-7.5F, 0.25F, -8.0F}}, 1.0F / 60.0F);
    world.simulate(1.0 / 60.0F);
    expect_near(world.body_transform(kinematic_component.body()).position.x,
                -7.5F, 0.01F,
                "Velocity-derived kinematic movement did not reach its target");
    expect_throws([&] {
        world.move_kinematic_body(visitor.body(), {}, 1.0F / 60.0F);
    }, "Dynamic body incorrectly accepted kinematic movement");
    expect(world.body_owner(trigger.body()) == trigger_entity &&
               world.body_owner(visitor.body()) == visitor_entity,
           "Body-to-entity lookup did not preserve logical ownership");

    world.set_linear_velocity(visitor.body(), {2.0F, 0.0F, 0.0F});
    std::uint32_t enter_events = 0;
    std::uint32_t stay_events = 0;
    std::uint32_t exit_events = 0;
    std::uint64_t previous_event_step = 0;
    for (std::uint32_t step = 0; step < 150; ++step) {
        world.simulate(1.0 / 60.0);
        for (const auto& event : world.take_trigger_events()) {
            expect(event.simulation_step >= previous_event_step,
                   "Trigger events were not drained in deterministic step order");
            previous_event_step = event.simulation_step;
            expect(event.trigger == trigger_entity && event.other == visitor_entity &&
                       event.trigger_body == trigger.body() &&
                       event.other_body == visitor.body(),
                   "Jolt contact callback lost logical entity/body identity");
            enter_events += event.type == gloom::physics::TriggerEventType::entered ? 1U : 0U;
            stay_events += event.type == gloom::physics::TriggerEventType::stayed ? 1U : 0U;
            exit_events += event.type == gloom::physics::TriggerEventType::exited ? 1U : 0U;
        }
    }
    expect(enter_events == 1 && stay_events > 0 && exit_events == 1,
           "Jolt sensor did not aggregate one queued enter/exit pair with stay events");
    expect(world.take_trigger_events().empty(),
           "Draining trigger events did not consume the queue");
    expect(world.body_transform(visitor.body()).position.x > 1.0F,
           "Trigger sensor produced a collision response instead of allowing passage");

    const auto trigger_body = trigger.body();
    expect(entities.destroy(trigger_entity),
           "Destroying a logical trigger entity failed");
    expect_throws([&] { static_cast<void>(world.body_transform(trigger_body)); },
                  "Destroying a trigger component left its Jolt sensor alive");
    expect(entities.remove<gloom::physics::StaticBodyComponent>(static_entity),
           "Static physics component could not be removed independently");
    entities.clear();
    expect(entities.size() == 0,
           "Clearing logical entities did not release physics components");

    const auto previous_world_generation = world.instance_generation();
    const auto old_world_entity = entities.create();
    entities.emplace<gloom::physics::DynamicBodyComponent>(
        old_world_entity,
        world,
        old_world_entity,
        gloom::physics::BodyDesc{
            .shape = {.type = gloom::physics::ShapeType::sphere, .radius = 0.2F},
            .transform = {.position = {4.0F, 4.0F, 4.0F}},
        });

    world.destroy_body(sphere);
    world.destroy_character(character);
    world.destroy_body(floor);
    world.stop();

    world.start();
    expect(world.instance_generation() != previous_world_generation,
           "Restarted Jolt world did not advance its instance generation");
    const auto replacement_body = world.create_body(gloom::physics::BodyDesc{
        .shape = {.type = gloom::physics::ShapeType::sphere, .radius = 0.2F},
        .transform = {.position = {1.0F, 1.0F, 1.0F}},
        .motion = gloom::physics::MotionType::dynamic,
    });
    entities.clear();
    expect_near(world.body_transform(replacement_body).position.x,
                1.0F,
                0.001F,
                "A stale component destroyed a body from a restarted Jolt world");
    world.destroy_body(replacement_body);
    world.stop();

    expect(world.state() == gloom::core::SubsystemState::stopped, "Jolt world did not stop");
    std::cout << "Gloom Jolt physics tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Physics test failure: " << error.what() << '\n';
    return 1;
}
