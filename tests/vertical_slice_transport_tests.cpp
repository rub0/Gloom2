#include <gloom/backends/gns_transport.hpp>
#include <gloom/gameplay/vertical_slice_network.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

void send(gloom::network::Transport& transport,
          const gloom::network::ConnectionId connection,
          const gloom::gameplay::SliceWireMessage& outgoing) {
    const auto bytes = gloom::network::encode_message(outgoing.message);
    transport.send(connection, {.payload = bytes, .delivery = outgoing.delivery});
}

void send(gloom::network::Transport& transport,
          const gloom::gameplay::SliceHostMessage& outgoing) {
    send(transport, outgoing.connection, outgoing);
}

void test_two_clients(bool original_factory=false) {
    using namespace std::chrono_literals;
    gloom::backends::GnsTransport server_transport;
    gloom::backends::GnsTransport first_transport;
    gloom::backends::GnsTransport second_transport;
    gloom::gameplay::VerticalSliceRemoteHost host{
        gloom::gameplay::SliceRemoteHostSettings{.original_factory=original_factory}};
    const auto initial=host.snapshot();
    gloom::gameplay::VerticalSliceRemoteClient first{
        original_factory ? gloom::gameplay::SlicePlayerSelection{.character=gloom::gameplay::SliceCharacter::archangel,
                             .ability=gloom::gameplay::SliceAbility::diamond_skin}
                         : gloom::gameplay::SlicePlayerSelection{},true};
    gloom::gameplay::VerticalSliceRemoteClient second{
        original_factory ? gloom::gameplay::SlicePlayerSelection{.character=gloom::gameplay::SliceCharacter::shadow,
                             .ability=gloom::gameplay::SliceAbility::invisibility}
                         : gloom::gameplay::SlicePlayerSelection{},true};
    server_transport.start();
    first_transport.start();
    second_transport.start();
    const auto endpoint = server_transport.listen("127.0.0.1:0");
    const auto first_connection = first_transport.connect(endpoint);
    const auto second_connection = second_transport.connect(endpoint);
    bool first_hello = false;
    bool second_hello = false;
    std::uint32_t simulated_ticks = 0;

    for (std::uint32_t pump = 0;
         pump < 5'000 && (!first.has_snapshot() || !second.has_snapshot() ||
                          simulated_ticks < 120);
         ++pump) {
        server_transport.tick(1.0 / 60.0);
        first_transport.tick(1.0 / 60.0);
        second_transport.tick(1.0 / 60.0);
        const double now = static_cast<double>(pump) / 1000.0;
        while (auto event = server_transport.poll_event()) {
            if (event->incoming && event->state == gloom::network::ConnectionState::connected) {
                host.connected(event->connection);
            }
        }
        const auto admit_on_connect = [&](gloom::backends::GnsTransport& transport,
                                          gloom::gameplay::VerticalSliceRemoteClient& client,
                                          const gloom::network::ConnectionId connection,
                                          bool& sent_hello) {
            while (auto event = transport.poll_event()) {
                if (!event->incoming && event->connection == connection &&
                    event->state == gloom::network::ConnectionState::connected && !sent_hello) {
                    send(transport, connection, client.begin());
                    sent_hello = true;
                }
            }
        };
        admit_on_connect(first_transport, first, first_connection, first_hello);
        admit_on_connect(second_transport, second, second_connection, second_hello);
        while (auto packet = server_transport.receive()) {
            const auto message = gloom::network::decode_message(packet->payload);
            expect(message.has_value(), "Two-client GNS host received malformed traffic");
            for (const auto& response : host.receive(packet->connection, *message, now)) {
                send(server_transport, response);
            }
        }
        const auto receive_client = [&](gloom::backends::GnsTransport& transport,
                                        gloom::gameplay::VerticalSliceRemoteClient& client,
                                        const gloom::network::ConnectionId connection) {
            while (auto packet = transport.receive()) {
                const auto message = gloom::network::decode_message(packet->payload);
                expect(message.has_value(), "Two-client GNS client received malformed traffic");
                if (auto response = client.receive(*message, now)) {
                    send(transport, connection, *response);
                }
            }
        };
        receive_client(first_transport, first, first_connection);
        receive_client(second_transport, second, second_connection);

        if (first.active() && second.active()) {
            const bool match_active =
                host.lobby().phase == gloom::gameplay::SliceMatchPhase::active;
            for (const auto& outgoing : first.create_input({.axis_z = -1.0F})) {
                send(first_transport, first_connection, outgoing);
            }
            // Original spawn 2 faces a crate along +Z; cross its open walkway along +X.
            const gloom::gameplay::SliceInput second_input{
                .axis_x = original_factory ? 1.0F : 0.0F,
                .axis_z = original_factory ? 0.0F : 1.0F,
            };
            for (const auto& outgoing : second.create_input(second_input)) {
                send(second_transport, second_connection, outgoing);
            }
            for (const auto& snapshot : host.tick_clients()) {
                send(server_transport, snapshot);
            }
            if (match_active) {
                ++simulated_ticks;
            }
        }
        std::this_thread::sleep_for(1ms);
    }

    expect(first.has_snapshot() && second.has_snapshot() && host.active_clients() == 2,
           "Two real GNS clients did not join the authoritative slice");
    expect(first.snapshot().player.entity != second.snapshot().player.entity &&
               first.snapshot().opponent.entity == second.snapshot().player.entity &&
               second.snapshot().opponent.entity == first.snapshot().player.entity,
           "Two real GNS clients did not receive reciprocal perspectives");
    const float second_distance = original_factory
        ? second.snapshot().player.position_x - initial.opponent.position_x
        : second.snapshot().player.position_z - initial.opponent.position_z;
    expect(first.snapshot().player.position_z < initial.player.position_z-2.0F &&
               second_distance > 2.0F,
           "Two real GNS clients did not move independently");
    expect(first.snapshot().scene_id==host.snapshot().scene_id && second.snapshot().scene_id==host.snapshot().scene_id,
           "GNS clients negotiated different Factory collision data");
    if (original_factory) expect(first.snapshot().opponent.character==gloom::gameplay::SliceCharacter::shadow &&
        second.snapshot().opponent.character==gloom::gameplay::SliceCharacter::archangel,"Original character identity lost across GNS");

    first_transport.disconnect(first_connection);
    second_transport.disconnect(second_connection);
    first_transport.stop();
    second_transport.stop();
    server_transport.stop();
}

} // namespace

int main() try {
    using namespace std::chrono_literals;
    gloom::backends::GnsTransport server_transport;
    gloom::backends::GnsTransport client_transport;
    gloom::gameplay::VerticalSliceRemoteHost host{false};
    gloom::gameplay::VerticalSliceRemoteClient client;
    server_transport.start();
    client_transport.start();
    const auto endpoint = server_transport.listen("127.0.0.1:0");
    const auto client_connection = client_transport.connect(endpoint);

    std::uint32_t simulated_ticks = 0;
    bool sent_hello = false;
    for (std::uint32_t pump = 0;
         pump < 5'000 && (!client.has_snapshot() || simulated_ticks < 120);
         ++pump) {
        server_transport.tick(1.0 / 60.0);
        client_transport.tick(1.0 / 60.0);
        const double now = static_cast<double>(pump) / 1000.0;

        while (auto event = server_transport.poll_event()) {
            if (event->incoming && event->state == gloom::network::ConnectionState::connected) {
                host.connected(event->connection);
            } else if (event->incoming &&
                       (event->state == gloom::network::ConnectionState::disconnected ||
                        event->state == gloom::network::ConnectionState::failed)) {
                host.disconnected(event->connection);
            }
        }
        while (auto event = client_transport.poll_event()) {
            if (!event->incoming && event->connection == client_connection &&
                event->state == gloom::network::ConnectionState::connected && !sent_hello) {
                send(client_transport, client_connection, client.begin());
                sent_hello = true;
            }
        }
        while (auto packet = server_transport.receive()) {
            const auto decoded = gloom::network::decode_message(packet->payload);
            expect(decoded.has_value(), "GNS host received a malformed slice message");
            for (const auto& response : host.receive(packet->connection, *decoded, now)) {
                send(server_transport, response);
            }
        }
        while (auto packet = client_transport.receive()) {
            const auto decoded = gloom::network::decode_message(packet->payload);
            expect(decoded.has_value(), "GNS client received a malformed slice message");
            if (auto response = client.receive(*decoded, now)) {
                send(client_transport, client_connection, *response);
            }
        }

        if (client.active()) {
            const bool match_active =
                host.lobby().phase == gloom::gameplay::SliceMatchPhase::active;
            const gloom::gameplay::SliceInput input{
                .axis_z = simulated_ticks < 60 ? -1.0F : 0.0F,
                .jump = simulated_ticks == 20,
            };
            for (const auto& outgoing : client.create_input(input)) {
                send(client_transport, client_connection, outgoing);
            }
            if (auto snapshot = host.tick()) {
                send(server_transport, *snapshot);
            }
            if (match_active) {
                ++simulated_ticks;
            }
        }
        std::this_thread::sleep_for(1ms);
    }

    expect(sent_hello && client.active() && host.active_clients() == 1,
           "Real GNS slice host/join handshake did not complete");
    expect(host.lobby().phase == gloom::gameplay::SliceMatchPhase::active,
           "Real GNS slice lobby did not become active after selection");
    expect(client.has_snapshot() && client.snapshot().simulation_tick >= 100,
           "Real GNS slice client did not receive authoritative snapshots");
    expect(host.snapshot().player.position_z < -1.0F,
           "Real GNS slice input did not move the authoritative player");
    expect(client_transport.metrics().sent_packets > 100 &&
               server_transport.metrics().sent_packets > 20,
           "Real GNS slice exchange did not carry sustained gameplay traffic");

    client_transport.disconnect(client_connection);
    client_transport.stop();
    server_transport.stop();
    test_two_clients();
    test_two_clients(true);
    std::cout << "Gloom GNS vertical-slice host/join tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "GNS vertical-slice test failure: " << error.what() << '\n';
    return 1;
}
