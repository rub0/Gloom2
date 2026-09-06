#include <gloom/network/clock_sync.hpp>
#include <gloom/network/movement_replication.hpp>
#include <gloom/network/network_simulator.hpp>
#include <gloom/network/presentation_smoother.hpp>
#include <gloom/network/protocol.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

void test_clock_synchronization() {
    gloom::network::ClockSynchronizer clock;
    clock.observe({.nonce = 1,
                   .client_send_seconds = 10.350,
                   .server_receive_seconds = 10.040,
                   .server_send_seconds = 10.045,
                   .client_receive_seconds = 10.435});
    expect(std::abs(clock.estimate().offset_seconds + 0.350) < 0.000'001,
           "Clock offset estimate is incorrect");
    expect(std::abs(clock.estimate().round_trip_seconds - 0.080) < 0.000'001,
           "Clock round-trip estimate is incorrect");
    expect(std::abs(clock.server_time(20.350) - 20.0) < 0.000'001,
           "Clock conversion did not produce server time");

    // A much slower, asymmetric sample should have limited influence.
    clock.observe({.nonce = 2,
                   .client_send_seconds = 30.350,
                   .server_receive_seconds = 30.050,
                   .server_send_seconds = 30.055,
                   .client_receive_seconds = 30.855});
    expect(std::abs(clock.estimate().offset_seconds + 0.350) < 0.02,
           "High-delay clock sample biased the estimate excessively");
}

void test_movement_wire_schemas() {
    const gloom::network::MovementInput input{
        .sequence = 77,
        .simulation_tick = 123,
        .axis_x = 0.5F,
        .axis_z = -0.25F,
        .jump = true};
    const auto input_message = gloom::network::encode_movement_input(input);
    const auto encoded_input = gloom::network::encode_message(input_message);
    const auto decoded_envelope = gloom::network::decode_message(encoded_input);
    expect(decoded_envelope.has_value(), "Movement input envelope was rejected");
    const auto decoded_input = gloom::network::decode_movement_input(*decoded_envelope);
    expect(decoded_input && decoded_input->sequence == input.sequence &&
               decoded_input->simulation_tick == input.simulation_tick &&
               decoded_input->axis_x == input.axis_x && decoded_input->axis_z == input.axis_z &&
               decoded_input->jump,
           "Movement input round trip changed values");

    const std::array batch_inputs{
        gloom::network::MovementInput{
            .sequence = 78, .simulation_tick = 124, .axis_x = 0.25F},
        gloom::network::MovementInput{
            .sequence = 79, .simulation_tick = 125, .axis_z = 1.0F, .jump = true},
    };
    const auto batch_message = gloom::network::encode_movement_input_batch(batch_inputs);
    const auto decoded_batch = gloom::network::decode_movement_input_batch(batch_message);
    expect(decoded_batch && decoded_batch->size() == 2 &&
               decoded_batch->front().sequence == 78 && decoded_batch->back().jump &&
               batch_message.sequence == 79,
           "Movement input batch round trip changed values");
    expect(!gloom::network::decode_movement_input(batch_message),
           "Single-input decoder accepted a redundant batch");
    auto malformed_batch = batch_message;
    malformed_batch.payload.front() = std::byte{0};
    expect(!gloom::network::decode_movement_input_batch(malformed_batch),
           "Empty movement input batch was accepted");

    const gloom::network::WorldSnapshot snapshot{
        .simulation_tick = 456,
        .acknowledged_input = 75,
        .entities = {{.entity = 1,
                      .simulation_tick = 456,
                      .position_x = 2.0F,
                      .position_y = 0.75F,
                      .position_z = 3.0F,
                      .velocity_x = 4.0F,
                      .velocity_y = 1.0F,
                      .velocity_z = 5.0F,.air_dodge_available=true},
                     {.entity = 2,
                      .simulation_tick = 456,
                      .position_x = -2.0F,
                      .position_z = -3.0F,
                      .velocity_x = -4.0F,
                      .velocity_z = -5.0F}},
    };
    const auto snapshot_message = gloom::network::encode_world_snapshot(snapshot, 9);
    const auto decoded_snapshot = gloom::network::decode_world_snapshot(snapshot_message);
    expect(decoded_snapshot && decoded_snapshot->simulation_tick == 456 &&
               decoded_snapshot->acknowledged_input == 75 &&
               decoded_snapshot->entities.size() == 2 && decoded_snapshot->entities[0].air_dodge_available &&
               decoded_snapshot->entities[1].position_z == -3.0F,
           "World snapshot round trip changed values");

    auto delta_state = snapshot;
    delta_state.entities.front().air_dodge_available=false;
    delta_state.simulation_tick = 457;
    delta_state.acknowledged_input = 76;
    delta_state.entities.front().position_x = 2.123F;
    delta_state.entities.erase(delta_state.entities.begin() + 1);
    delta_state.entities.push_back({.entity = 3, .position_x = 8.0F});
    const auto delta_message =
        gloom::network::encode_world_snapshot_delta(delta_state, 10, snapshot, 9);
    expect(delta_message.payload.size() <
               gloom::network::encode_world_snapshot(delta_state, 10).payload.size(),
           "Delta snapshot was not smaller than the equivalent full snapshot");
    expect(!gloom::network::decode_world_snapshot(delta_message),
           "Delta snapshot decoded without its baseline");
    const auto decoded_delta = gloom::network::decode_world_snapshot(delta_message, &snapshot);
    expect(decoded_delta && decoded_delta->entities.size() == 2 && !decoded_delta->entities.front().air_dodge_available &&
               decoded_delta->entities.front().entity == 1 &&
               std::abs(decoded_delta->entities.front().position_x - 2.123F) < 0.0021F &&
               decoded_delta->entities.back().entity == 3,
           "Delta snapshot did not reconstruct changed, removed and created entities");
    const auto baseline_sequence =
        gloom::network::world_snapshot_baseline_sequence(delta_message);
    expect(baseline_sequence && *baseline_sequence == 9,
           "Delta snapshot baseline sequence was not encoded");

    auto malformed = snapshot_message;
    malformed.payload.pop_back();
    expect(!gloom::network::decode_world_snapshot(malformed),
           "Truncated world snapshot was accepted");

    const gloom::network::ReplicationSettings settings{};
    gloom::network::PredictedMovementClient client{
        settings, 1, gloom::network::MovementState{.entity = 1}};
    client.receive(snapshot_message);
    auto stale_snapshot = snapshot;
    stale_snapshot.entities.front().position_x = 100.0F;
    client.receive(gloom::network::encode_world_snapshot(stale_snapshot, 8));
    expect(client.local_state().position_x == 2.0F && client.reconciliation_count() == 1,
           "Out-of-order authoritative snapshot was applied");
}

void test_redundant_input_recovers_lost_jump() {
    const gloom::network::ReplicationSettings settings{
        .tick_rate = 60,
        .snapshot_rate = 60,
        .input_redundancy = 4,
    };
    const gloom::network::MovementState initial{.entity = 1};
    gloom::network::AuthoritativeMovementServer server{settings, 1};
    server.add_entity(initial);
    gloom::network::PredictedMovementClient client{settings, 1, initial};

    for (std::uint32_t sequence = 1; sequence <= 9; ++sequence) {
        server.receive(client.create_input(0.0F, 0.0F));
        const auto snapshot = server.tick();
        expect(snapshot.has_value(), "Per-tick test snapshot was not generated");
        client.receive(*snapshot);
    }

    // Deliberately discard the datagram that first carries the jump action.
    static_cast<void>(client.create_input(0.0F, 0.0F, true));
    const auto missed_jump_snapshot = server.tick();
    expect(missed_jump_snapshot.has_value(), "Lost-jump test snapshot was not generated");
    client.receive(*missed_jump_snapshot);
    expect(server.entity(1).grounded, "Server jumped without receiving the action");

    // The next unreliable datagram includes both the new command and input 10.
    server.receive(client.create_input(0.0F, 0.0F));
    const auto recovered_snapshot = server.tick();
    expect(recovered_snapshot.has_value(), "Recovered-jump test snapshot was not generated");
    expect(!server.entity(1).grounded && server.entity(1).position_y > 0.0F,
           "Redundant command did not recover the lost jump action");
    expect(server.acknowledged_input() == 11 &&
               server.input_metrics().redundant_commands >= 1 &&
               server.input_metrics().maximum_batch_size >= 2,
           "Redundant input telemetry did not record recovery");
    client.receive(*recovered_snapshot);
    expect(client.pending_input_count() == 0,
           "Recovered redundant commands were not acknowledged");
}

void test_authoritative_teleport_resets_motion() {
    const gloom::network::ReplicationSettings settings{.snapshot_rate = 60};
    gloom::network::AuthoritativeMovementServer server{settings, 1};
    server.add_entity({.entity = 1});
    server.set_entity_input(1, 1.0F, 0.0F);
    static_cast<void>(server.tick());
    expect(server.entity(1).velocity_x > 0.0F,
           "Authoritative teleport setup did not create motion");

    server.teleport_entity(1, -5.0F, 0.0F, 2.0F);
    const auto& teleported = server.entity(1);
    expect(teleported.position_x == -5.0F && teleported.position_y == 0.0F &&
               teleported.position_z == 2.0F && teleported.velocity_x == 0.0F &&
               teleported.velocity_y == 0.0F && teleported.velocity_z == 0.0F &&
               teleported.grounded,
           "Authoritative teleport did not reset position and motion atomically");
}

void test_snapshot_relevance_and_baselines() {
    const gloom::network::ReplicationSettings settings{
        .tick_rate = 60,
        .snapshot_rate = 60,
        .relevance_radius = 10.0F,
        .maximum_snapshot_entities = 2,
    };
    gloom::network::AuthoritativeMovementServer server{settings, 1};
    server.add_entity({.entity = 1});
    server.add_entity({.entity = 2, .position_x = 3.0F});
    server.add_entity({.entity = 3, .position_x = 6.0F});
    server.add_entity({.entity = 4, .position_x = 20.0F});
    gloom::network::PredictedMovementClient client{settings, 1, {.entity = 1}};

    server.receive(client.create_input(0.0F, 0.0F));
    const auto first = server.tick();
    expect(first.has_value() && first->flags == gloom::network::MessageFlags::full_snapshot,
           "First relevant snapshot was not full");
    const auto first_state = gloom::network::decode_world_snapshot(*first);
    expect(first_state && first_state->entities.size() == 2 &&
               first_state->entities[0].entity == 1 && first_state->entities[1].entity == 2,
           "Snapshot relevance did not prioritize controlled and nearest entities");
    client.receive(*first);

    server.receive(client.create_input(0.0F, 0.0F));
    const auto second = server.tick();
    expect(second.has_value() && second->flags == gloom::network::MessageFlags::delta_snapshot,
           "Acknowledged snapshot baseline did not produce a delta");
    const auto second_baseline = gloom::network::world_snapshot_baseline_sequence(*second);
    expect(second_baseline && *second_baseline == first->sequence,
           "Delta did not reference the client's acknowledged baseline");
    client.receive(*second);
    expect(server.snapshot_metrics().full_snapshots == 1 &&
               server.snapshot_metrics().delta_snapshots == 1 &&
               server.snapshot_metrics().entities_considered == 8,
           "Snapshot relevance/baseline telemetry is incorrect");
}

void test_predicted_character_gameplay() {
    const gloom::network::ReplicationSettings settings{
        .movement_speed = 5.0F,
        .jump_speed = 5.5F,
        .gravity = -15.0F,
        .character_radius = 0.4F,
        .static_obstacles = {{.minimum_x = 1.0F,
                              .maximum_x = 2.0F,
                              .minimum_z = -1.0F,
                              .maximum_z = 1.0F,
                              .top_y = 0.6F}},
    };
    constexpr double fixed_delta = 1.0 / 60.0;
    gloom::network::MovementState blocked{.entity = 1};
    for (std::uint64_t tick = 1; tick <= 60; ++tick) {
        blocked = gloom::network::simulate_movement(
            blocked,
            {.sequence = static_cast<std::uint32_t>(tick),
             .simulation_tick = tick,
             .axis_x = 1.0F},
            fixed_delta,
            settings);
    }
    expect(blocked.position_x < 0.61F && blocked.grounded,
           "Deterministic static obstacle did not block grounded movement");

    gloom::network::MovementState jumping{.entity = 1};
    float maximum_height = 0.0F;
    for (std::uint64_t tick = 1; tick <= 150; ++tick) {
        jumping = gloom::network::simulate_movement(
            jumping,
            {.sequence = static_cast<std::uint32_t>(tick),
             .simulation_tick = tick,
             .axis_x = 1.0F,
             .jump = tick == 1},
            fixed_delta,
            settings);
        maximum_height = std::max(maximum_height, jumping.position_y);
    }
    expect(maximum_height > 0.8F, "Predicted jump did not leave the ground");
    expect(jumping.position_x > 2.4F,
           "Airborne deterministic character did not clear the low obstacle");
    expect(jumping.grounded && std::abs(jumping.position_y) < 0.001F,
           "Predicted character did not land on the ground");

    gloom::network::PresentationSmoother smoother;
    const gloom::network::ReconciliationMetrics correction{
        .count = 1, .last_distance = 0.2F, .last_delta_x = 0.2F};
    smoother.observe_correction(correction);
    const auto continuous = smoother.apply({.entity = 1, .position_x = 1.0F});
    expect(std::abs(continuous.position_x - 0.8F) < 0.001F,
           "Presentation correction did not preserve visual continuity");
    smoother.advance(0.1);
    const auto half_decayed = smoother.apply({.entity = 1, .position_x = 1.0F});
    expect(std::abs(half_decayed.position_x - 0.9F) < 0.001F,
           "Presentation correction did not decay by its configured half-life");
    smoother.observe_correction(
        {.count = 2, .last_distance = 2.0F, .last_delta_x = 2.0F});
    expect(smoother.metrics().smoothed_corrections == 1 &&
               smoother.metrics().snapped_corrections == 1,
           "Presentation smoothing telemetry is incorrect");
}

void submit(gloom::network::NetworkSimulator& link,
            const std::uint64_t flow,
            const gloom::network::ProtocolMessage& message) {
    const auto encoded = gloom::network::encode_message(message);
    link.submit({.flow = flow, .sequence = message.sequence, .payload = encoded});
}

void test_predicted_combatant_collision_and_respawn_reset() {
    const gloom::network::ReplicationSettings settings{
        .tick_rate = 60,
        .snapshot_rate = 20,
        .movement_speed = 5.0F,
        .character_radius = 0.4F,
        .character_collision_height = 1.8F,
        .resolve_entity_collisions = true,
    };
    const gloom::network::MovementState local{.entity = 1};
    const gloom::network::MovementState remote{.entity = 2, .position_x = 0.7F};
    gloom::network::AuthoritativeMovementServer server{settings, 1};
    server.add_entity(local);
    server.add_entity(remote);
    gloom::network::PredictedMovementClient client{settings, 1, local};
    const std::array collision{remote};
    client.set_collision_entities(collision);

    const auto input = client.create_input(1.0F, 0.0F);
    server.receive(input);
    static_cast<void>(server.tick());
    expect(std::abs(client.local_state().position_x - server.entity(1).position_x) < 0.001F,
           "Client did not predict the server's combatant collision displacement");

    static_cast<void>(client.create_input(1.0F, 0.0F));
    expect(client.pending_input_count() != 0,
           "Respawn reset test did not create pending prediction");
    client.reset_local_state({.entity = 1, .simulation_tick = 40, .position_x = -5.0F});
    expect(client.pending_input_count() == 0 &&
               std::abs(client.local_state().position_x + 5.0F) < 0.001F,
           "Respawn reset replayed stale pre-respawn inputs");
}

template <typename Receiver>
void drain(gloom::network::NetworkSimulator& link, Receiver&& receiver) {
    while (const auto packet = link.receive()) {
        const auto message = gloom::network::decode_message(packet->payload);
        if (!message) {
            throw std::runtime_error{"Simulated link corrupted a protocol message"};
        }
        receiver(*message);
    }
}

void test_authoritative_vertical_slice() {
    const gloom::network::ReplicationSettings settings{
        .tick_rate = 60,
        .snapshot_rate = 20,
        .movement_speed = 6.0F,
        .interpolation_delay_seconds = 0.12,
    };
    constexpr gloom::network::NetworkEntityId local_id = 1;
    constexpr gloom::network::NetworkEntityId remote_id = 2;
    const gloom::network::MovementState local_initial{.entity = local_id};
    const gloom::network::MovementState remote_initial{.entity = remote_id};

    gloom::network::AuthoritativeMovementServer server{settings, local_id};
    server.add_entity(local_initial);
    server.add_entity(remote_initial);
    server.set_entity_input(remote_id, 0.0F, 0.5F);
    gloom::network::PredictedMovementClient client{settings, local_id, local_initial};

    const gloom::network::NetworkSimulationSettings adverse{
        .latency_seconds = 0.045,
        .jitter_seconds = 0.018,
        .loss_probability = 0.05,
        .duplicate_probability = 0.02,
        .reorder_probability = 0.10,
        .maximum_reorder_delay_seconds = 0.035,
        .bandwidth_bytes_per_second = 32 * 1024,
        .queue_capacity_packets = 256,
        .random_seed = 0x5eed,
    };
    auto downstream_settings = adverse;
    downstream_settings.random_seed = 0xc0ffee;
    gloom::network::NetworkSimulator upstream{adverse};
    gloom::network::NetworkSimulator downstream{downstream_settings};

    constexpr double fixed_delta = 1.0 / 60.0;
    std::uint64_t interpolated_samples = 0;
    float maximum_correction = 0.0F;
    for (std::uint64_t tick = 0; tick < 600; ++tick) {
        float axis = 0.0F;
        if (tick < 180) {
            axis = 1.0F;
        } else if (tick >= 300 && tick < 480) {
            axis = -0.5F;
        }
        submit(upstream, 1, client.create_input(axis, 0.0F));
        upstream.advance(fixed_delta);
        downstream.advance(fixed_delta);
        drain(upstream, [&](const auto& message) { server.receive(message); });
        if (const auto snapshot = server.tick()) {
            submit(downstream, 2, *snapshot);
        }
        drain(downstream, [&](const auto& message) {
            client.receive(message);
            maximum_correction = std::max(maximum_correction, client.last_correction_distance());
        });
        const double server_time = static_cast<double>(server.simulation_tick()) /
                                   static_cast<double>(settings.tick_rate);
        if (const auto remote = client.sample_remote(remote_id, server_time);
            remote && remote->mode == gloom::network::SnapshotSampleMode::interpolated) {
            ++interpolated_samples;
        }
    }

    // Stop producing client inputs, then let the last command, acknowledgement and
    // snapshots traverse both links so final states can be compared authoritatively.
    for (std::uint64_t tick = 0; tick < 120; ++tick) {
        upstream.advance(fixed_delta);
        downstream.advance(fixed_delta);
        drain(upstream, [&](const auto& message) { server.receive(message); });
        if (const auto snapshot = server.tick()) {
            submit(downstream, 2, *snapshot);
        }
        drain(downstream, [&](const auto& message) { client.receive(message); });
    }
    downstream.advance(1.0);
    drain(downstream, [&](const auto& message) { client.receive(message); });

    const auto& authoritative = server.entity(local_id);
    const auto& predicted = client.local_state();
    expect(std::abs(predicted.position_x - authoritative.position_x) < 0.005F &&
               std::abs(predicted.position_z - authoritative.position_z) < 0.005F,
           "Predicted client did not converge to authoritative movement");
    expect(client.pending_input_count() == 0,
           "Acknowledged client inputs remained in prediction history");
    expect(client.reconciliation_count() > 100 && maximum_correction > 0.001F,
           "Adverse link did not exercise prediction reconciliation");
    expect(interpolated_samples > 300,
           "Remote entity was not predominantly snapshot-interpolated");
    expect(upstream.metrics().dropped_by_loss > 0 && downstream.metrics().dropped_by_loss > 0,
           "Deterministic adverse test did not exercise packet loss");
    expect(server.snapshot_metrics().full_snapshots > 0 &&
               server.snapshot_metrics().delta_snapshots > 0 &&
               server.snapshot_metrics().payload_bytes > 0,
           "Adverse link did not exercise full and delta snapshot paths");

    const double server_time = static_cast<double>(server.simulation_tick()) /
                               static_cast<double>(settings.tick_rate);
    const auto remote = client.sample_remote(remote_id, server_time);
    expect(remote && remote->state.position_z > 0.0F,
           "Remote authoritative entity was not available to render");
}

} // namespace

int main() try {
    test_clock_synchronization();
    test_movement_wire_schemas();
    test_predicted_character_gameplay();
    test_redundant_input_recovers_lost_jump();
    test_authoritative_teleport_resets_motion();
    test_snapshot_relevance_and_baselines();
    test_predicted_combatant_collision_and_respawn_reset();
    test_authoritative_vertical_slice();
    std::cout << "Gloom authoritative replication tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Network replication test failure: " << error.what() << '\n';
    return 1;
}
