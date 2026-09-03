#include <gloom/network/combat.hpp>

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

gloom::network::WorldSnapshot frame(const std::uint64_t tick,
                                    const float target_z) {
    return {
        .simulation_tick = tick,
        .entities = {
            {.entity = 1},
            {.entity = 2, .position_x = 5.0F, .position_z = target_z},
            {.entity = 3, .position_x = 3.0F, .position_z = 2.0F},
        },
    };
}

void test_fire_wire_schema() {
    const gloom::network::FireCommand command{
        .sequence = 17,
        .shooter = 1,
        .estimated_server_tick = 42,
        .aim_x = 1.0F,
        .maximum_distance = 25.0F,
    };
    const auto encoded_command = gloom::network::encode_fire_command(command);
    const auto decoded_command = gloom::network::decode_fire_command(encoded_command);
    expect(decoded_command && decoded_command->sequence == 17 &&
               decoded_command->shooter == 1 &&
               decoded_command->estimated_server_tick == 42 &&
               decoded_command->maximum_distance == 25.0F,
           "Fire command wire round trip changed values");

    const gloom::network::FireResult result{
        .fire_sequence = 17,
        .shooter = 1,
        .target = 2,
        .requested_tick = 42,
        .evaluated_tick = 42,
        .server_tick = 48,
        .distance = 4.5F,
        .status = gloom::network::FireValidationStatus::hit,
    };
    const auto encoded_result = gloom::network::encode_fire_result(result);
    const auto decoded_result = gloom::network::decode_fire_result(encoded_result);
    expect(decoded_result && decoded_result->fire_sequence == 17 &&
               decoded_result->target == 2 && decoded_result->server_tick == 48 &&
               decoded_result->status == gloom::network::FireValidationStatus::hit,
           "Fire result wire round trip changed values");

    auto malformed = encoded_command;
    malformed.payload.front() = std::byte{99};
    expect(!gloom::network::decode_fire_command(malformed),
           "Unknown combat event subtype was accepted");
}

void test_lag_compensated_validation() {
    gloom::network::LagCompensatedCombatServer combat{
        {.history_ticks = 6, .future_tolerance_ticks = 2, .maximum_range = 20.0F}};
    for (std::uint64_t tick = 1; tick <= 10; ++tick) {
        combat.record(frame(tick, static_cast<float>(tick) - 5.0F));
    }
    expect(combat.history_size() == 6, "Combat history did not enforce its tick budget");

    const auto rewound_hit = combat.validate({
        .sequence = 1,
        .shooter = 1,
        .estimated_server_tick = 5,
        .aim_x = 1.0F,
        .maximum_distance = 10.0F,
    });
    expect(rewound_hit.status == gloom::network::FireValidationStatus::hit &&
               rewound_hit.target == 2 && rewound_hit.evaluated_tick == 5 &&
               std::abs(rewound_hit.distance - 4.5F) < 0.001F,
           "Rewound hitscan did not hit the historical target");

    const auto current_miss = combat.validate({
        .sequence = 2,
        .shooter = 1,
        .estimated_server_tick = 10,
        .aim_x = 1.0F,
        .maximum_distance = 10.0F,
    });
    expect(current_miss.status == gloom::network::FireValidationStatus::miss,
           "Current-time hitscan unexpectedly hit a target that moved away");

    expect(combat.validate({.sequence = 2,
                            .shooter = 1,
                            .estimated_server_tick = 10,
                            .aim_x = 1.0F,
                            .maximum_distance = 10.0F})
               .status == gloom::network::FireValidationStatus::rejected_duplicate,
           "Duplicate fire sequence was accepted");
    expect(combat.validate({.sequence = 3,
                            .shooter = 1,
                            .estimated_server_tick = 20,
                            .aim_x = 1.0F,
                            .maximum_distance = 10.0F})
               .status == gloom::network::FireValidationStatus::rejected_future,
           "Far-future fire command was accepted");
    expect(combat.validate({.sequence = 3,
                            .shooter = 1,
                            .estimated_server_tick = 4,
                            .aim_x = 1.0F,
                            .maximum_distance = 10.0F})
               .status == gloom::network::FireValidationStatus::rejected_too_old,
           "Expired fire command was accepted");

    const auto& metrics = combat.metrics();
    expect(metrics.hits == 1 && metrics.misses == 1 &&
               metrics.duplicate_rejections == 1 && metrics.future_rejections == 1 &&
               metrics.too_old_rejections == 1,
           "Combat validation telemetry is incorrect");
}

void test_world_occlusion() {
    const gloom::network::CombatSettings settings{
        .history_ticks = 4,
        .maximum_range = 20.0F,
        .static_obstacles = {{.minimum_x = 2.0F,
                              .maximum_x = 3.0F,
                              .minimum_z = -1.0F,
                              .maximum_z = 1.0F,
                              .top_y = 2.0F}},
    };
    gloom::network::LagCompensatedCombatServer combat{settings};
    combat.record(frame(1, 0.0F));
    const auto result = combat.validate({
        .sequence = 1,
        .shooter = 1,
        .estimated_server_tick = 1,
        .aim_x = 1.0F,
        .maximum_distance = 10.0F,
    });
    expect(result.status == gloom::network::FireValidationStatus::miss,
           "Hitscan passed through authoritative static world geometry");
}

void test_nearest_target_and_invalid_shooter() {
    gloom::network::LagCompensatedCombatServer combat{{.history_ticks = 4}};
    combat.record({
        .simulation_tick = 1,
        .entities = {
            {.entity = 1},
            {.entity = 2, .position_x = 5.0F},
            {.entity = 3, .position_x = 3.0F},
        },
    });
    const auto nearest = combat.validate({
        .sequence = 1,
        .shooter = 1,
        .estimated_server_tick = 1,
        .aim_x = 1.0F,
        .maximum_distance = 10.0F,
    });
    expect(nearest.status == gloom::network::FireValidationStatus::hit && nearest.target == 3,
           "Hitscan did not select the nearest historical target");

    const auto missing = combat.validate({
        .sequence = 7,
        .shooter = 4,
        .estimated_server_tick = 1,
        .aim_x = 1.0F,
        .maximum_distance = 10.0F,
    });
    expect(missing.status == gloom::network::FireValidationStatus::rejected_invalid,
           "A shooter absent from the historical frame was accepted");
    combat.record({
        .simulation_tick = 2,
        .entities = {{.entity = 4}},
    });
    const auto retry = combat.validate({
        .sequence = 7,
        .shooter = 4,
        .estimated_server_tick = 2,
        .aim_x = 1.0F,
        .maximum_distance = 10.0F,
    });
    expect(retry.status == gloom::network::FireValidationStatus::miss,
           "An invalid shooter command incorrectly consumed its sequence number");
}

void test_vertical_capsule_matches_character_silhouette() {
    gloom::network::LagCompensatedCombatServer combat{{.history_ticks = 4,
                                                       .hit_radius = 0.55F,
                                                       .character_center_height = 0.9F,
                                                       .hit_half_height = 0.45F,
                                                       .maximum_range = 20.0F}};
    combat.record(frame(1, 0.0F));
    const float target_x = 5.0F;
    const float target_y = 0.8F;
    const float inverse_length = 1.0F / std::sqrt(target_x * target_x + target_y * target_y);
    const auto upper_body_hit = combat.validate({
        .sequence = 1,
        .shooter = 1,
        .estimated_server_tick = 1,
        .aim_x = target_x * inverse_length,
        .aim_y = target_y * inverse_length,
        .maximum_distance = 10.0F,
    });
    expect(upper_body_hit.status == gloom::network::FireValidationStatus::hit &&
               upper_body_hit.target == 2,
           "Upper character silhouette was outside its authoritative hit volume");
}

} // namespace

int main() try {
    test_fire_wire_schema();
    test_lag_compensated_validation();
    test_world_occlusion();
    test_nearest_target_and_invalid_shooter();
    test_vertical_capsule_matches_character_silhouette();
    std::cout << "Gloom lag-compensated combat tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Combat test failure: " << error.what() << '\n';
    return 1;
}
