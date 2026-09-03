#include <gloom/gameplay/vertical_slice_network.hpp>
#include <gloom/gameplay/factory_scene.hpp>
#include <gloom/network/network_simulator.hpp>

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

class TestIdentityProvider final : public gloom::gameplay::SliceIdentityProvider {
public:
    mutable std::uint32_t verification_count{0};

    [[nodiscard]] std::expected<gloom::gameplay::SlicePlayerIdentity, std::string>
    verify(const std::string_view credential) const override {
        ++verification_count;
        if (credential != "opaque-provider-token") {
            return std::unexpected{"Untrusted token"};
        }
        return gloom::gameplay::SlicePlayerIdentity{
            .account_id = 9001, .display_name = "Verified"};
    }
};

[[nodiscard]] gloom::network::ProtocolMessage
wire(const gloom::network::ProtocolMessage& message) {
    const auto decoded = gloom::network::decode_message(gloom::network::encode_message(message));
    if (!decoded) {
        throw std::runtime_error{"Remote slice wire round trip failed"};
    }
    return *decoded;
}

void admit(gloom::gameplay::VerticalSliceRemoteHost& host,
           gloom::gameplay::VerticalSliceRemoteClient& client,
           const gloom::network::ConnectionId connection,
           const double now,
           const bool reconnect = false) {
    host.connected(connection);
    const auto hello = reconnect ? client.reconnect() : client.begin();
    auto responses = host.receive(connection, wire(hello.message), now);
    expect(responses.size() == 1, "Remote slice client admission failed");
    auto clock_request = client.receive(wire(responses.front().message), now + 0.001);
    expect(clock_request.has_value(), "Remote slice client did not request clock sync");
    responses = host.receive(connection, wire(clock_request->message), now + 0.002);
    expect(responses.size() == 1, "Remote slice host did not answer client clock sync");
    const auto selection = client.receive(wire(responses.front().message), now + 0.003);
    if (reconnect) {
        return;
    }
    expect(selection.has_value() &&
               gloom::gameplay::decode_lobby_selection(selection->message).has_value(),
           "Remote slice client did not submit its default selection");
    static_cast<void>(host.receive(connection, wire(selection->message), now + 0.004));
    const auto ready = client.receive(
        wire(gloom::gameplay::encode_lobby_state(host.lobby())), now + 0.005);
    expect(ready.has_value() &&
               gloom::gameplay::decode_lobby_ready(ready->message).value_or(false),
           "Remote slice client did not enter the lobby after selection");
    static_cast<void>(host.receive(connection, wire(ready->message), now + 0.006));
}

void test_interactive_selection_ordering() {
    using namespace gloom::gameplay;
    constexpr gloom::network::ConnectionId connection = 71;
    VerticalSliceRemoteHost host{SliceRemoteHostSettings{}};
    VerticalSliceRemoteClient client{
        {.account_id = 7071, .display_name = "Chooser"}, {}, false};
    host.connected(connection);
    auto responses = host.receive(connection, wire(client.begin().message), 0.0);
    expect(responses.size() == 1, "Interactive client admission failed");
    auto request = client.receive(wire(responses.front().message), 0.001);
    expect(request.has_value(), "Interactive client omitted clock request");
    responses = host.receive(connection, wire(request->message), 0.002);
    expect(responses.size() == 1 &&
               !client.receive(wire(responses.front().message), 0.003),
           "Interactive client submitted a selection before confirmation");
    const SlicePlayerSelection guard{.ability = SliceAbility::guard};
    expect(client.set_desired_selection(guard) &&
               client.desired_selection() == guard,
           "Interactive client could not change its pending loadout");
    const auto selection = client.confirm_selection();
    expect(selection && decode_lobby_selection(selection->message) == guard,
           "Interactive confirmation did not emit the chosen loadout");
    static_cast<void>(host.receive(connection, wire(selection->message), 0.004));
    expect(!client.set_desired_selection({.ability = SliceAbility::none}) &&
               !client.confirm_selection(),
           "Interactive selection changed after confirmation");
    const auto ready = client.receive(wire(encode_lobby_state(host.lobby())), 0.005);
    expect(ready && decode_lobby_ready(ready->message).value_or(false),
           "Interactive client sent ready before authoritative selection confirmation");
}

void test_original_factory_network() {
    using namespace gloom::gameplay;
    VerticalSliceRemoteHost host{SliceRemoteHostSettings{.required_players=1,.original_factory=true}};
    VerticalSliceRemoteClient client;
    constexpr gloom::network::ConnectionId connection=178;
    admit(host,client,connection,0.0);
    for (unsigned tick=0;tick<180;++tick) {
        const auto now=tick/60.0;
        for (const auto& input:client.create_input({.axis_z=-.5F,.jump=tick==60}))
            static_cast<void>(host.receive(connection,wire(input.message),now));
        for (const auto& message:host.tick_clients()) {
            if (const auto reply=client.receive(wire(message.message),now))
                static_cast<void>(host.receive(connection,wire(reply->message),now));
        }
    }
    expect(client.has_snapshot() && client.snapshot().scene_id==original_factory().scene_id,"Factory scene negotiation failed");
    expect(host.snapshot().player.alive && client.snapshot().player.alive,"Factory remote spawn is not playable");
    expect(std::abs(client.snapshot().player.position_z-host.snapshot().player.position_z)<.5F &&
        std::abs(client.snapshot().player.position_y-host.snapshot().player.position_y)<.5F,"Factory prediction and authority diverged");
    host.disconnected(connection);
    admit(host,client,connection,4.0,true);
    for (unsigned tick=0;tick<10;++tick) for (const auto& message:host.tick_clients())
        static_cast<void>(client.receive(wire(message.message),4.1+tick/60.0));
    expect(client.snapshot().scene_id==original_factory().scene_id && host.active_clients()==1,"Factory reconnect changed scene");
}

void test_injected_verified_identity() {
    using namespace gloom::gameplay;
    constexpr gloom::network::ConnectionId connection = 72;
    const auto provider = std::make_shared<TestIdentityProvider>();
    VerticalSliceRemoteHost host{SliceRemoteHostSettings{
        .required_players = 1,
        .identity_provider = provider}};
    VerticalSliceRemoteClient client{
        {.account_id = 1234, .display_name = "UntrustedClaim"}};
    host.connected(connection);
    const auto rejected = host.receive(
        connection, wire(client.begin_with_credential("invalid-token").message), 0.0);
    expect(rejected.empty() && host.lobby().players.empty(),
           "Injected identity provider admitted an invalid opaque token");

    const auto accepted = host.receive(
        connection,
        wire(client.begin_with_credential("opaque-provider-token").message), 0.001);
    expect(accepted.size() == 1 && host.lobby().players.size() == 1 &&
               host.lobby().players.front().identity.account_id == 9001 &&
               host.lobby().players.front().identity.display_name == "Verified",
           "Lobby used client-asserted identity instead of the verified principal");
    expect(provider->verification_count == 2,
           "Host verified an opaque credential more than once per hello");
}

void test_two_players_and_reconnect() {
    using namespace gloom::gameplay;
    constexpr gloom::network::ConnectionId first_connection = 81;
    constexpr gloom::network::ConnectionId second_connection = 82;
    constexpr gloom::network::ConnectionId resumed_connection = 83;
    VerticalSliceRemoteHost host{SliceRemoteHostSettings{}};
    VerticalSliceRemoteClient first{
        {.account_id = 1001, .display_name = "BerserkerPlayer"},
        {.character = SliceCharacter::berserker, .ability = SliceAbility::none}};
    VerticalSliceRemoteClient second{
        {.account_id = 2002, .display_name = "GuardPlayer"},
        {.ability = SliceAbility::guard}};
    admit(host, first, first_connection, 0.0);
    admit(host, second, second_connection, 0.01);
    expect(host.lobby().phase == SliceMatchPhase::active,
           "Selections and ready commands did not start the remote lobby");
    expect(first.session().controlled_entity() == VerticalSliceSimulation::player_entity &&
               second.session().controlled_entity() == VerticalSliceSimulation::opponent_entity,
           "Two remote clients did not receive distinct combatants");

    for (std::uint32_t tick = 0; tick < 120; ++tick) {
        for (const auto& outgoing :
             first.create_input({.axis_z = tick < 45 ? -1.0F : 0.0F})) {
            static_cast<void>(host.receive(first_connection, wire(outgoing.message),
                                           static_cast<double>(tick) / 60.0));
        }
        const auto second_view = second.has_snapshot()
                                     ? second.snapshot()
                                     : host.snapshot();
        const float aim_x = second_view.opponent.position_x -
                            second_view.player.position_x;
        const float aim_y = second_view.opponent.position_y -
                            second_view.player.position_y;
        const float aim_z = second_view.opponent.position_z -
                            second_view.player.position_z;
        const float aim_length = std::sqrt(aim_x * aim_x + aim_y * aim_y + aim_z * aim_z);
        for (const auto& outgoing : second.create_input({
                 .axis_z = tick < 45 ? -0.5F : 0.0F,
                 .aim_x = aim_x / aim_length,
                 .aim_y = aim_y / aim_length,
                 .aim_z = aim_z / aim_length,
                 .fire_primary = tick >= 45,
                 .use_primary_ability = tick == 10,
             })) {
            static_cast<void>(host.receive(second_connection, wire(outgoing.message),
                                           static_cast<double>(tick) / 60.0));
        }
        for (const auto& snapshot : host.tick_clients()) {
            auto& client = snapshot.connection == first_connection ? first : second;
            static_cast<void>(client.receive(wire(snapshot.message),
                                             static_cast<double>(tick + 1) / 60.0));
        }
    }

    expect(host.active_clients() == 2 && first.has_snapshot() && second.has_snapshot(),
           "Two remote players did not receive sustained authoritative state");
    expect(first.snapshot().player.entity == VerticalSliceSimulation::player_entity &&
               first.snapshot().opponent.entity == VerticalSliceSimulation::opponent_entity &&
               second.snapshot().player.entity == VerticalSliceSimulation::opponent_entity &&
               second.snapshot().opponent.entity == VerticalSliceSimulation::player_entity &&
               first.snapshot().player.character == SliceCharacter::berserker &&
               second.snapshot().opponent.character == SliceCharacter::berserker &&
               first.snapshot().player.ability == SliceAbility::none &&
               second.snapshot().player.ability == SliceAbility::guard &&
               first.snapshot().opponent.ability == SliceAbility::guard,
           "Per-client snapshots did not place the controlled combatant first");
    expect(second.snapshot().player.shield == hound_guard_shield &&
               host.snapshot().network.authorized_ability_commands == 1,
           "Guard selection was not authorized and replicated over the remote slice");
    expect(first.snapshot().player.position_z < -2.0F &&
               second.snapshot().player.position_z < -1.0F &&
               std::abs(first.snapshot().player.position_z -
                        second.snapshot().player.position_z) > 0.5F,
           "Remote combatants did not move independently");
    expect(first.snapshot().factory_lift.entity == 100 &&
               second.snapshot().factory_lift.entity == 100 &&
               first.snapshot().factory_lift.position_y > 0.25F &&
               std::abs(first.snapshot().factory_lift.position_y -
                        second.snapshot().factory_lift.position_y) < 0.001F,
           "Factory kinematic state was not replicated consistently to both clients");
    expect(second.snapshot().player.kills != 0 && !first.snapshot().player.alive,
           "The second remote player could not kill the first through authority");

    const auto resumed_entity = second.session().controlled_entity();
    host.disconnected(second_connection);
    expect(host.active_clients() == 1, "Disconnected player remained active");
    admit(host, second, resumed_connection, 2.5, true);
    expect(host.active_clients() == 2 && second.session().controlled_entity() == resumed_entity &&
               host.session_metrics().resumed == 1,
           "Remote player did not resume the same authoritative combatant");

    for (const auto& outgoing : second.create_input({.axis_x = -1.0F})) {
        static_cast<void>(host.receive(resumed_connection, wire(outgoing.message), 2.51));
    }
    for (const auto& snapshot : host.tick_clients()) {
        if (snapshot.connection == resumed_connection) {
            static_cast<void>(second.receive(wire(snapshot.message), 2.52));
        }
    }
    expect(host.session_metrics().unauthorized_messages == 0,
           "Resumed player traffic failed ownership authorization");
}

void test_host_abandonment_outcome() {
    using namespace gloom::gameplay;
    constexpr gloom::network::ConnectionId first_connection = 91;
    constexpr gloom::network::ConnectionId second_connection = 92;
    VerticalSliceRemoteHost host{SliceRemoteHostSettings{
        .reconnect_grace_ticks = 3,
        .validate_identity = valid_development_identity,
    }};
    VerticalSliceRemoteClient first{{.account_id = 91, .display_name = "First"}};
    VerticalSliceRemoteClient second{{.account_id = 92, .display_name = "Second"}};
    admit(host, first, first_connection, 0.0);
    admit(host, second, second_connection, 0.01);
    for (const auto& message : host.tick_clients()) {
        auto& client = message.connection == first_connection ? first : second;
        static_cast<void>(client.receive(wire(message.message), 0.02));
    }
    expect(host.lobby().phase == SliceMatchPhase::active,
           "Remote host did not enter the active lobby phase");

    host.disconnected(second_connection);
    for (std::uint32_t tick = 0; tick < 4; ++tick) {
        for (const auto& message : host.tick_clients()) {
            if (message.connection == first_connection) {
                static_cast<void>(first.receive(wire(message.message),
                                                0.03 + tick / 60.0));
            }
        }
    }
    expect(host.lobby().phase == SliceMatchPhase::completed &&
               host.lobby().end_reason == SliceMatchEndReason::abandonment &&
               host.lobby().winner_entity == first.session().controlled_entity() &&
               first.lobby().phase == SliceMatchPhase::completed,
           "Remote host did not replicate the abandonment outcome");
}

void test_adverse_gameplay_link() {
    using namespace gloom::gameplay;
    constexpr gloom::network::ConnectionId connection = 73;
    constexpr double fixed_delta = 1.0 / 60.0;
    constexpr std::uint32_t duration_ticks = 900;
    const gloom::network::NetworkSimulationSettings adverse{
        .latency_seconds = 0.045,
        .jitter_seconds = 0.018,
        .loss_probability = 0.05,
        .duplicate_probability = 0.01,
        .reorder_probability = 0.04,
        .maximum_reorder_delay_seconds = 0.035,
        .bandwidth_bytes_per_second = 32'000,
        .queue_capacity_packets = 256,
        .random_seed = 0x20'25'08'30,
    };
    auto downstream_settings = adverse;
    downstream_settings.random_seed ^= 0x9e37'79b9;
    gloom::network::NetworkSimulator upstream{adverse};
    gloom::network::NetworkSimulator downstream{downstream_settings};
    VerticalSliceRemoteHost host{false};
    VerticalSliceRemoteClient client;

    admit(host, client, connection, 0.0);

    std::uint64_t upstream_sequence = 0;
    std::uint64_t downstream_sequence = 0;
    bool saw_kill = false;
    bool saw_respawn = false;
    bool opponent_was_dead = false;
    for (std::uint32_t tick = 0; tick < duration_ticks; ++tick) {
        upstream.advance(fixed_delta);
        downstream.advance(fixed_delta);
        const double now = static_cast<double>(tick + 1) * fixed_delta;

        while (auto packet = upstream.receive()) {
            const auto decoded = gloom::network::decode_message(packet->payload);
            expect(decoded.has_value(), "Adverse upstream corrupted a slice message");
            const auto direct = host.receive(connection, *decoded, now);
            expect(direct.empty(), "Gameplay traffic unexpectedly generated a response");
        }
        while (auto packet = downstream.receive()) {
            const auto decoded = gloom::network::decode_message(packet->payload);
            expect(decoded.has_value(), "Adverse downstream corrupted a slice snapshot");
            static_cast<void>(client.receive(*decoded, now));
        }

        SliceInput input{.axis_z = tick < 120 ? -1.0F : 0.0F,
                         .jump = tick == 30,
                         .fire_primary = tick >= 120};
        const auto& state = client.has_snapshot() ? client.snapshot() : host.snapshot();
        const float x = state.opponent.position_x - state.player.position_x;
        const float y = state.opponent.position_y - state.player.position_y;
        const float z = state.opponent.position_z - state.player.position_z;
        const float length = std::sqrt(x * x + y * y + z * z);
        if (length > 1.0e-5F) {
            input.aim_x = x / length;
            input.aim_y = y / length;
            input.aim_z = z / length;
        }
        for (const auto& outgoing : client.create_input(input)) {
            const auto bytes = gloom::network::encode_message(outgoing.message);
            upstream.submit({.flow = connection,
                             .sequence = ++upstream_sequence,
                             .payload = bytes});
        }
        if (auto snapshot = host.tick()) {
            const auto bytes = gloom::network::encode_message(snapshot->message);
            downstream.submit({.flow = connection,
                               .sequence = ++downstream_sequence,
                               .payload = bytes});
        }

        const auto& authoritative = host.snapshot();
        saw_kill = saw_kill || authoritative.player.kills != 0;
        saw_respawn = saw_respawn || (opponent_was_dead && authoritative.opponent.alive);
        opponent_was_dead = !authoritative.opponent.alive;
    }

    const auto upstream_metrics = upstream.metrics();
    const auto downstream_metrics = downstream.metrics();
    expect(upstream_metrics.dropped_by_loss > 0 &&
               downstream_metrics.dropped_by_loss > 0,
           "Adverse slice link did not exercise packet loss in both directions");
    expect(upstream_metrics.duplicated_packets > 0 ||
               downstream_metrics.duplicated_packets > 0,
           "Adverse slice link did not exercise packet duplication");
    expect(saw_kill && saw_respawn,
           "Adverse slice link did not preserve the kill/respawn loop");
    expect(host.snapshot().player.position_z < -2.0F,
           "Adverse slice link did not preserve authoritative movement");
    expect(client.reconciliation_metrics().count > 100,
           "Adverse slice link did not exercise prediction reconciliation");
    expect(client.reconciliation_metrics().maximum_distance < 0.75F,
           "Adverse slice link caused an excessive prediction correction");
    expect(host.session_metrics().unauthorized_messages == 0,
           "Adverse slice traffic caused an authorization failure");

    std::cout << "Adverse slice link: max correction "
              << client.reconciliation_metrics().maximum_distance
              << ", accumulated "
              << client.reconciliation_metrics().accumulated_distance
              << ", upstream loss " << upstream_metrics.dropped_by_loss
              << ", downstream loss " << downstream_metrics.dropped_by_loss << '\n';
}

} // namespace

int main() try {
    using namespace gloom::gameplay;
    constexpr gloom::network::ConnectionId connection = 41;
    constexpr double fixed_delta = 1.0 / 60.0;

    VerticalSliceRemoteHost host{false};
    VerticalSliceRemoteClient client;
    host.connected(connection);
    auto responses = host.receive(connection, wire(client.begin().message), 0.0);
    expect(responses.size() == 1 &&
               responses.front().delivery == gloom::network::Delivery::reliable,
           "Remote slice host did not admit the client reliably");
    auto clock_request = client.receive(wire(responses.front().message), 0.001);
    expect(client.active() && clock_request.has_value(),
           "Remote slice client did not accept its session");
    responses = host.receive(connection, wire(clock_request->message), 0.002);
    expect(responses.size() == 1, "Remote slice host did not answer clock synchronization");
    const auto selection = client.receive(wire(responses.front().message), 0.003);
    expect(selection.has_value() && decode_lobby_selection(selection->message).has_value(),
           "Remote slice client did not submit its selection");
    static_cast<void>(host.receive(connection, wire(selection->message), 0.004));
    const auto ready = client.receive(wire(encode_lobby_state(host.lobby())), 0.005);
    expect(ready.has_value() && decode_lobby_ready(ready->message).value_or(false),
           "Remote slice client did not become lobby-ready");
    static_cast<void>(host.receive(connection, wire(ready->message), 0.006));
    expect(client.session().clock().estimate().samples == 1,
           "Remote slice clock exchange did not update the estimator");

    bool saw_kill = false;
    bool saw_respawn = false;
    bool opponent_was_dead = false;
    std::uint32_t sent_fire_messages = 0;
    std::uint32_t sent_ability_messages = 0;
    for (std::uint32_t tick = 0; tick < 360; ++tick) {
        SliceInput input{.axis_z = tick < 45 ? -1.0F : 0.0F,
                         .jump = tick == 12,
                         .fire_primary = tick >= 45,
                         .use_primary_ability = tick == 100};
        const auto& state = client.has_snapshot() ? client.snapshot() : host.snapshot();
        const float x = state.opponent.position_x - state.player.position_x;
        const float y = state.opponent.position_y - state.player.position_y;
        const float z = state.opponent.position_z - state.player.position_z;
        const float length = std::sqrt(x * x + y * y + z * z);
        if (length > 1.0e-5F) {
            input.aim_x = x / length;
            input.aim_y = y / length;
            input.aim_z = z / length;
        }
        // Deliver client commands in two-command bursts. The host must consume
        // and acknowledge them one simulation step at a time instead of
        // acknowledging unsimulated prediction history.
        const std::uint32_t burst_count = tick % 2U == 0U ? 2U : 0U;
        for (std::uint32_t burst = 0; burst < burst_count; ++burst) {
            for (const auto& outgoing : client.create_input(input)) {
                if (outgoing.message.kind == gloom::network::MessageKind::event) {
                    ++sent_fire_messages;
                } else if (outgoing.message.kind ==
                           gloom::network::MessageKind::ability_command) {
                    ++sent_ability_messages;
                    expect(outgoing.delivery == gloom::network::Delivery::reliable,
                           "One-shot Hound bite was sent unreliably");
                }
                const auto ignored = host.receive(connection, wire(outgoing.message),
                                                   static_cast<double>(tick) * fixed_delta);
                expect(ignored.empty(),
                       "Gameplay command unexpectedly generated a direct response");
            }
        }
        if (auto snapshot = host.tick()) {
            static_cast<void>(client.receive(wire(snapshot->message),
                                             static_cast<double>(tick + 1) * fixed_delta));
        }
        const auto& authoritative = host.snapshot();
        saw_kill = saw_kill || authoritative.player.kills != 0;
        saw_respawn = saw_respawn || (opponent_was_dead && authoritative.opponent.alive);
        opponent_was_dead = !authoritative.opponent.alive;
    }

    expect(client.has_snapshot(), "Remote slice client never received gameplay state");
    expect(saw_kill && saw_respawn,
           "Remote authoritative slice did not complete the kill/respawn loop");
    expect(host.active_clients() == 1 && host.session_metrics().unauthorized_messages == 0,
           "Remote slice session authorization reported an unexpected failure");
    expect(client.snapshot().network.reconciliation_count > 0,
           "Remote slice client did not reconcile prediction");
    expect(client.reconciliation_metrics().maximum_distance < 0.02F,
           "Burst delivery caused visible remote prediction corrections");
    expect(sent_fire_messages > 0 && sent_fire_messages < 20,
           "Held fire flooded the remote transport instead of respecting cooldown");
    expect(sent_ability_messages == 1 &&
               host.snapshot().network.authorized_ability_commands == 1,
           "Remote Hound bite was not sequenced and authorized exactly once");
    expect(client.snapshot().player.character == SliceCharacter::hound &&
               client.snapshot().hud.primary_ability_ready_fraction < 1.0F,
           "Remote snapshot did not replicate Hound and its ability cooldown");

    auto dead_snapshot = host.snapshot();
    dead_snapshot.player.life = 0.0F;
    dead_snapshot.player.alive = false;
    dead_snapshot.hud.dead = true;
    static_cast<void>(client.receive(
        wire(encode_slice_snapshot(dead_snapshot, 0, 10'000)), 6.5));
    const auto dead_commands = client.create_input({.axis_x = 1.0F,
                                                     .aim_x = 1.0F,
                                                     .jump = true,
                                                     .fire_primary = true});
    expect(dead_commands.size() == 1,
           "Dead remote player emitted a fire command");
    const auto dead_inputs =
        gloom::network::decode_movement_input_batch(dead_commands.front().message);
    expect(dead_inputs && dead_inputs->back().axis_x == 0.0F &&
               dead_inputs->back().axis_z == 0.0F && !dead_inputs->back().jump,
           "Dead remote player continued local movement prediction");

    auto spoofed = gloom::network::encode_fire_command({.sequence = 10'000,
                                                         .shooter = 2,
                                                         .estimated_server_tick =
                                                             host.snapshot().simulation_tick,
                                                         .aim_x = 1.0F,
                                                         .maximum_distance = soul_reaper_range});
    static_cast<void>(host.receive(connection, wire(spoofed), 7.0));
    expect(host.session_metrics().unauthorized_messages == 1,
           "Remote slice host accepted a spoofed shooter entity");

    host.disconnected(connection);
    expect(host.active_clients() == 0,
           "Remote slice session remained active after disconnect");
    test_interactive_selection_ordering();
    test_injected_verified_identity();
    test_two_players_and_reconnect();
    test_host_abandonment_outcome();
    test_adverse_gameplay_link();
    test_original_factory_network();
    std::cout << "Gloom remote vertical-slice tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Remote vertical-slice test failure: " << error.what() << '\n';
    return 1;
}
