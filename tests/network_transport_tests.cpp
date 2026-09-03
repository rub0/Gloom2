#include <gloom/backends/gns_transport.hpp>
#include <gloom/network/combat.hpp>
#include <gloom/network/movement_replication.hpp>
#include <gloom/network/protocol.hpp>
#include <gloom/network/session.hpp>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

template <typename Predicate>
void pump_until(gloom::backends::GnsTransport& server,
                gloom::backends::GnsTransport& client,
                Predicate&& predicate,
                const char* timeout_message) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
    while (!predicate()) {
        server.tick(0.0);
        client.tick(0.0);
        if (std::chrono::steady_clock::now() >= deadline) {
            throw std::runtime_error{timeout_message};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
}

} // namespace

int main() try {
    gloom::backends::GnsTransport server;
    gloom::backends::GnsTransport client;
    server.start();
    client.start();

    const std::string endpoint = server.listen("127.0.0.1:0");
    expect(!endpoint.empty() && !endpoint.ends_with(":0"), "Server did not bind an ephemeral port");
    const auto client_connection = client.connect(endpoint);
    expect(client_connection != gloom::network::invalid_connection, "Client connection ID is invalid");

    bool client_connected = false;
    bool server_connected = false;
    gloom::network::ConnectionId server_connection = gloom::network::invalid_connection;
    pump_until(
        server,
        client,
        [&] {
            while (auto event = server.poll_event()) {
                if (event->state == gloom::network::ConnectionState::connected) {
                    server_connected = true;
                    server_connection = event->connection;
                    expect(event->incoming, "Server connection was not marked incoming");
                }
            }
            while (auto event = client.poll_event()) {
                if (event->state == gloom::network::ConnectionState::connected) {
                    client_connected = true;
                    expect(!event->incoming, "Client connection was marked incoming");
                }
            }
            return client_connected && server_connected;
        },
        "Timed out establishing loopback client/server connection");

    std::uint64_t next_resume_token = 100;
    gloom::network::ServerSessionManager server_sessions({
        .authenticate = [](const std::string_view credential) {
            return credential == "loopback";
        },
        .generate_resume_token = [&next_resume_token] { return ++next_resume_token; },
    });
    gloom::network::ClientSession client_session;
    server_sessions.connected(server_connection);
    const auto hello_message = gloom::network::encode_message(
        client_session.begin("loopback", 42));
    client.send(client_connection,
                {.payload = hello_message, .delivery = gloom::network::Delivery::reliable});
    pump_until(
        server,
        client,
        [&] {
            while (auto packet = server.receive()) {
                const auto envelope = gloom::network::decode_message(packet->payload);
                const auto hello = envelope
                                       ? gloom::network::decode_client_hello(*envelope)
                                       : std::expected<gloom::network::ClientHello, std::string>{
                                             std::unexpected{"Invalid envelope"}};
                expect(hello.has_value(), "Server did not decode the client hello");
                const auto welcome =
                    server_sessions.admit(packet->connection, *hello, 600, 10.0);
                expect(welcome.has_value(), "Server rejected the loopback session");
                const auto encoded = gloom::network::encode_message(
                    gloom::network::encode_server_welcome(*welcome));
                server.send(packet->connection,
                            {.payload = encoded,
                             .delivery = gloom::network::Delivery::reliable});
            }
            while (auto packet = client.receive()) {
                const auto envelope = gloom::network::decode_message(packet->payload);
                expect(envelope.has_value(), "Client did not decode the server welcome");
                client_session.accept(*envelope);
            }
            return client_session.active();
        },
        "Timed out completing the loopback session handshake");

    const auto clock_request = gloom::network::encode_message(
        client_session.create_clock_request(10.35));
    client.send(client_connection,
                {.payload = clock_request,
                 .delivery = gloom::network::Delivery::unreliable});
    bool clock_synchronized = false;
    pump_until(
        server,
        client,
        [&] {
            while (auto packet = server.receive()) {
                const auto envelope = gloom::network::decode_message(packet->payload);
                const auto request = envelope
                                         ? gloom::network::decode_clock_request(*envelope)
                                         : std::expected<gloom::network::ClockRequest, std::string>{
                                               std::unexpected{"Invalid envelope"}};
                expect(request.has_value(), "Server did not decode the clock request");
                const auto response = gloom::network::encode_message(
                    gloom::network::encode_clock_response({
                        .nonce = request->nonce,
                        .client_send_seconds = request->client_send_seconds,
                        .server_receive_seconds = 10.0,
                        .server_send_seconds = 10.01,
                    }));
                server.send(packet->connection,
                            {.payload = response,
                             .delivery = gloom::network::Delivery::unreliable});
            }
            while (auto packet = client.receive()) {
                const auto envelope = gloom::network::decode_message(packet->payload);
                expect(envelope.has_value(), "Client did not decode the clock response");
                client_session.receive_clock_response(*envelope, 10.45);
                clock_synchronized = true;
            }
            return clock_synchronized;
        },
        "Timed out synchronizing the loopback session clock");
    expect(client_session.clock().estimate().samples == 1,
           "Real loopback clock exchange did not update the estimator");

    const auto input_message = gloom::network::encode_message(
        gloom::network::encode_movement_input(
            {.sequence = 17, .simulation_tick = 20, .axis_x = 0.75F, .axis_z = -0.25F}));
    const auto snapshot_message = gloom::network::encode_message(
        gloom::network::encode_world_snapshot(
            {.simulation_tick = 21,
             .acknowledged_input = 17,
             .entities = {{.entity = 1,
                           .simulation_tick = 21,
                           .position_x = 4.0F,
                           .position_z = 2.0F,
                           .velocity_x = 1.0F,
                           .velocity_z = 0.0F}}},
            8));
    const auto fire_message = gloom::network::encode_message(
        gloom::network::encode_fire_command({.sequence = 18,
                                             .shooter = 1,
                                             .estimated_server_tick = 20,
                                             .aim_x = 1.0F,
                                             .maximum_distance = 50.0F}));
    client.send(client_connection,
                {.payload = input_message, .delivery = gloom::network::Delivery::reliable});
    client.send(client_connection,
                {.payload = fire_message, .delivery = gloom::network::Delivery::unreliable});
    server.send(server_connection,
                {.payload = snapshot_message, .delivery = gloom::network::Delivery::unreliable});

    bool server_received_input = false;
    bool server_received_fire = false;
    bool client_received = false;
    pump_until(
        server,
        client,
        [&] {
            while (auto packet = server.receive()) {
                const auto envelope = gloom::network::decode_message(packet->payload);
                expect(packet->connection == server_connection, "Server packet has wrong connection ID");
                if (envelope && envelope->kind == gloom::network::MessageKind::input_command) {
                    const auto input = gloom::network::decode_movement_input(*envelope);
                    const auto owner = server_sessions.authorize_input(
                        packet->connection, *envelope);
                    server_received_input = input && owner && *owner == 1 &&
                                            input->sequence == 17 && input->axis_x == 0.75F;
                    expect(packet->delivery == gloom::network::Delivery::reliable,
                           "Reliable input lost its delivery classification");
                } else if (envelope && envelope->kind == gloom::network::MessageKind::event) {
                    const auto fire = gloom::network::decode_fire_command(*envelope);
                    server_received_fire = fire && fire->sequence == 18 &&
                                           server_sessions.authorize_fire(
                                               packet->connection, *fire);
                    expect(packet->delivery == gloom::network::Delivery::unreliable,
                           "Unreliable fire event lost its delivery classification");
                }
            }
            while (auto packet = client.receive()) {
                const auto envelope = gloom::network::decode_message(packet->payload);
                const auto snapshot = envelope
                                          ? gloom::network::decode_world_snapshot(*envelope)
                                          : std::expected<gloom::network::WorldSnapshot, std::string>{
                                                std::unexpected{"Invalid envelope"}};
                client_received = snapshot && snapshot->acknowledged_input == 17 &&
                                  snapshot->entities.size() == 1 &&
                                  snapshot->entities.front().position_x == 4.0F;
                expect(packet->connection == client_connection, "Client packet has wrong connection ID");
                expect(packet->delivery == gloom::network::Delivery::unreliable,
                       "Unreliable packet lost its delivery classification");
            }
            return server_received_input && server_received_fire && client_received;
        },
        "Timed out exchanging reliable and unreliable loopback packets");

    const auto client_metrics = client.metrics();
    const auto server_metrics = server.metrics();
    expect(client_metrics.sent_packets == 4 && client_metrics.received_packets == 3,
           "Client packet metrics are incorrect");
    expect(server_metrics.sent_packets == 3 && server_metrics.received_packets == 4,
           "Server packet metrics are incorrect");

    client.disconnect(client_connection);
    bool server_saw_disconnect = false;
    pump_until(
        server,
        client,
        [&] {
            while (auto event = server.poll_event()) {
                if (event->connection == server_connection &&
                    event->state == gloom::network::ConnectionState::disconnected) {
                    server_saw_disconnect = true;
                }
            }
            return server_saw_disconnect;
        },
        "Server did not observe the client disconnect");

    client.stop();
    server.stop();
    std::cout << "Gloom GameNetworkingSockets loopback tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Network transport test failure: " << error.what() << '\n';
    return 1;
}
