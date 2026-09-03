#include <gloom/network/session.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

void test_session_wire_schemas() {
    const gloom::network::ClientHello hello{
        .client_nonce = 17,
        .resume_token = 23,
        .credential = "credential",
    };
    const auto decoded_hello =
        gloom::network::decode_client_hello(gloom::network::encode_client_hello(hello));
    expect(decoded_hello && decoded_hello->client_nonce == 17 &&
               decoded_hello->resume_token == 23 &&
               decoded_hello->credential == "credential",
           "Client hello wire round trip changed values");

    const gloom::network::ServerWelcome welcome{
        .client_nonce = 17,
        .session = 2,
        .controlled_entity = 4,
        .resume_token = 99,
        .server_tick = 120,
        .server_time_seconds = 2.0,
    };
    const auto encoded_welcome = gloom::network::encode_server_welcome(welcome);
    const auto envelope = gloom::network::decode_message(
        gloom::network::encode_message(encoded_welcome));
    expect(envelope.has_value(), "Protocol v6 rejected a server welcome envelope");
    const auto decoded_welcome = gloom::network::decode_server_welcome(*envelope);
    expect(decoded_welcome && decoded_welcome->session == 2 &&
               decoded_welcome->controlled_entity == 4 &&
               decoded_welcome->server_tick == 120,
           "Server welcome wire round trip changed values");

    const gloom::network::ClockResponse response{
        .nonce = 7,
        .client_send_seconds = 5.0,
        .server_receive_seconds = 4.8,
        .server_send_seconds = 4.81,
    };
    const auto decoded_response =
        gloom::network::decode_clock_response(gloom::network::encode_clock_response(response));
    expect(decoded_response && decoded_response->nonce == 7 &&
               decoded_response->server_send_seconds == 4.81,
           "Clock response wire round trip changed values");
}

void test_multiclient_admission_ownership_and_reconnect() {
    std::uint64_t next_token = 100;
    gloom::network::ServerSessionManager server({
        .maximum_clients = 2,
        .reconnect_grace_ticks = 10,
        .authenticate = [](const std::string_view credential) {
            return credential == "accepted";
        },
        .generate_resume_token = [&next_token] { return ++next_token; },
    });

    gloom::network::ClientSession first;
    server.connected(10);
    const auto rejected = server.admit(
        10, *gloom::network::decode_client_hello(first.begin("wrong", 1)), 100, 5.0);
    expect(!rejected, "Invalid session credential was accepted");
    const auto first_hello = gloom::network::decode_client_hello(first.begin("accepted", 2));
    const auto first_welcome = server.admit(10, *first_hello, 100, 5.0);
    expect(first_welcome.has_value(), "First client was not admitted");
    first.accept(gloom::network::encode_server_welcome(*first_welcome));

    gloom::network::ClientSession second;
    server.connected(20);
    const auto second_hello = gloom::network::decode_client_hello(second.begin("accepted", 3));
    const auto second_welcome = server.admit(20, *second_hello, 100, 5.0);
    expect(second_welcome && second_welcome->controlled_entity != first.controlled_entity(),
           "Two clients received the same controlled entity");
    second.accept(gloom::network::encode_server_welcome(*second_welcome));
    expect(server.active_sessions() == 2, "Server did not retain two active sessions");

    gloom::network::ProtocolMessage input{
        .kind = gloom::network::MessageKind::input_command,
        .acknowledged_sequence = 8,
    };
    const auto input_owner = server.authorize_input(10, input);
    expect(input_owner && *input_owner == first.controlled_entity(),
           "Input was not routed to its connection-owned entity");
    const auto* replication = server.replication_state(10);
    expect(replication && replication->has_acknowledged_snapshot &&
               replication->acknowledged_snapshot == 8 &&
               replication->received_input_batches == 1,
           "Per-client replication state was not updated");

    const gloom::network::FireCommand own_fire{
        .sequence = 1,
        .shooter = first.controlled_entity(),
        .estimated_server_tick = 100,
    };
    auto spoofed_fire = own_fire;
    spoofed_fire.shooter = second.controlled_entity();
    expect(server.authorize_fire(10, own_fire) && !server.authorize_fire(10, spoofed_fire),
           "Session ownership allowed a spoofed shooter entity");

    server.connected(30);
    const auto third = server.admit(
        30, {.client_nonce = 4, .credential = "accepted"}, 100, 5.0);
    expect(!third, "Session capacity limit was not enforced");

    const auto original_session = first.session();
    const auto original_entity = first.controlled_entity();
    const auto original_token = first.resume_token();
    server.disconnected(10, 100);
    expect(server.active_sessions() == 1 && server.dormant_sessions() == 1,
           "Disconnected session did not enter reconnect grace state");
    expect(!server.authorize_input(10, input),
           "Disconnected connection retained entity authority");

    server.connected(11);
    const auto resume_hello =
        gloom::network::decode_client_hello(first.reconnect("accepted", 5));
    expect(resume_hello && resume_hello->resume_token == original_token,
           "Client did not present its reconnect token");
    expect(!first.active(), "Client remained active while reconnect admission was pending");
    const auto resumed = server.admit(11, *resume_hello, 105, 5.1);
    expect(resumed && resumed->session == original_session &&
               resumed->controlled_entity == original_entity &&
               resumed->resume_token != original_token,
           "Reconnect did not preserve identity and rotate its token");
    first.accept(gloom::network::encode_server_welcome(*resumed));

    server.disconnected(11, 110);
    server.expire(121);
    expect(server.dormant_sessions() == 0 && server.metrics().expired == 1,
           "Expired reconnect state was not removed");
}

void test_live_clock_exchange() {
    std::uint64_t token = 500;
    gloom::network::ServerSessionManager server({
        .authenticate = [](std::string_view) { return true; },
        .generate_resume_token = [&token] { return ++token; },
    });
    gloom::network::ClientSession client;
    server.connected(1);
    const auto hello = gloom::network::decode_client_hello(client.begin("", 10));
    const auto welcome = server.admit(1, *hello, 600, 10.0);
    client.accept(gloom::network::encode_server_welcome(*welcome));

    const auto request_message = client.create_clock_request(10.35);
    const auto request = gloom::network::decode_clock_request(request_message);
    expect(request.has_value(), "Client did not create a valid clock request");
    const auto response = gloom::network::encode_clock_response({
        .nonce = request->nonce,
        .client_send_seconds = request->client_send_seconds,
        .server_receive_seconds = 10.0,
        .server_send_seconds = 10.01,
    });
    client.receive_clock_response(response, 10.45);
    const auto estimate = client.clock().estimate();
    expect(estimate.samples == 1 && std::abs(estimate.offset_seconds + 0.395) < 0.000'001 &&
               std::abs(estimate.round_trip_seconds - 0.09) < 0.000'001,
           "Live session clock exchange produced the wrong estimate");

    bool replay_rejected = false;
    try {
        client.receive_clock_response(response, 10.46);
    } catch (const std::invalid_argument&) {
        replay_rejected = true;
    }
    expect(replay_rejected, "Replayed clock response was accepted");
}

void test_per_client_replication_channels() {
    const gloom::network::ReplicationSettings settings{
        .tick_rate = 60,
        .snapshot_rate = 60,
        .movement_speed = 6.0F,
    };
    gloom::network::AuthoritativeMovementServer movement{settings, 1};
    movement.add_entity({.entity = 1});
    movement.add_entity({.entity = 2});
    movement.register_client(2);

    movement.receive(1, gloom::network::encode_movement_input({
                            .sequence = 10,
                            .axis_x = 1.0F,
                        }));
    movement.receive(2, gloom::network::encode_movement_input({
                            .sequence = 20,
                            .axis_x = -1.0F,
                        }));
    const auto first_snapshot = movement.tick();
    const auto second_snapshot = movement.take_snapshot(2);
    expect(first_snapshot && second_snapshot &&
               first_snapshot->flags == gloom::network::MessageFlags::full_snapshot &&
               second_snapshot->flags == gloom::network::MessageFlags::full_snapshot,
           "Initial per-client snapshots were not generated independently");
    expect(movement.acknowledged_input(1) == 10 && movement.acknowledged_input(2) == 20 &&
               movement.entity(1).position_x > 0.0F && movement.entity(2).position_x < 0.0F,
           "Inputs from two clients were not applied to their owned entities");

    auto first_input = gloom::network::encode_movement_input({
        .sequence = 11,
        .axis_x = 1.0F,
    });
    first_input.acknowledged_sequence = first_snapshot->sequence;
    movement.receive(1, first_input);
    movement.receive(2, gloom::network::encode_movement_input({
                            .sequence = 21,
                            .axis_x = -1.0F,
                        }));
    const auto first_delta = movement.tick();
    const auto second_full = movement.take_snapshot(2);
    expect(first_delta && second_full &&
               first_delta->flags == gloom::network::MessageFlags::delta_snapshot &&
               second_full->flags == gloom::network::MessageFlags::full_snapshot,
           "Snapshot acknowledgement from one client contaminated another baseline");
    expect(movement.snapshot_metrics(1).delta_snapshots == 1 &&
               movement.snapshot_metrics(2).delta_snapshots == 0,
           "Per-client snapshot metrics were not isolated");
}

void test_idempotent_event_delivery() {
    const gloom::network::ReliableEventSettings settings{
        .retry_interval_seconds = 0.1,
        .lifetime_seconds = 0.3,
        .maximum_pending = 4,
        .duplicate_window = 8,
    };
    gloom::network::ReliableEventSender sender{settings};
    gloom::network::ReliableEventReceiver receiver{settings.duplicate_window};
    const std::vector<std::byte> payload{std::byte{1}, std::byte{2}, std::byte{3}};
    const auto sequence = sender.queue(payload, 0.0);

    auto outgoing = sender.poll(0.0);
    expect(outgoing.size() == 1, "New gameplay event was not transmitted immediately");
    const auto first = receiver.receive(outgoing.front());
    expect(first && !first->duplicate && first->payload == payload,
           "Gameplay event was not delivered exactly once");
    expect(sender.poll(0.05).empty(), "Gameplay event retried before its deadline");

    outgoing = sender.poll(0.1);
    expect(outgoing.size() == 1, "Unacknowledged gameplay event was not retried");
    const auto duplicate = receiver.receive(outgoing.front());
    expect(duplicate && duplicate->duplicate && duplicate->payload.empty(),
           "Retransmitted gameplay event was delivered twice");
    expect(sender.acknowledge(receiver.acknowledgement(sequence)) && sender.pending() == 0,
           "Gameplay event acknowledgement did not clear the send queue");

    static_cast<void>(sender.queue(payload, 1.0));
    expect(sender.poll(1.31).empty() && sender.metrics().expired == 1,
           "Stale gameplay event was not expired");
    expect(sender.metrics().transmissions == 2 && sender.metrics().retransmissions == 1 &&
               receiver.metrics().delivered == 1 && receiver.metrics().duplicates == 1,
           "Reliable event telemetry is incorrect");
}

} // namespace

int main() try {
    test_session_wire_schemas();
    test_multiclient_admission_ownership_and_reconnect();
    test_live_clock_exchange();
    test_per_client_replication_channels();
    test_idempotent_event_delivery();
    std::cout << "Gloom multiclient session tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Session test failure: " << error.what() << '\n';
    return 1;
}
