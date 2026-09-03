#include <gloom/backends/gns_transport.hpp>
#include <gloom/backends/game_ticket.hpp>
#include <gloom/backends/jolt_world.hpp>
#include <gloom/backends/winhttp_match_service.hpp>
#include <gloom/gameplay/match_discovery.hpp>
#include <gloom/gameplay/vertical_slice_network.hpp>

#include <atomic>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace {

std::atomic_bool running{true};

void stop_server(int) { running.store(false, std::memory_order_relaxed); }

[[nodiscard]] std::uint64_t monotonic_milliseconds() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

void send(gloom::network::Transport& transport,
          const gloom::gameplay::SliceHostMessage& outgoing) {
    const auto bytes = gloom::network::encode_message(outgoing.message);
    transport.send(outgoing.connection,
                   {.payload = bytes, .delivery = outgoing.delivery});
}

void print_lobby(const gloom::gameplay::SliceLobbyState& lobby) {
    const char* phase = lobby.phase == gloom::gameplay::SliceMatchPhase::waiting
                            ? "waiting"
                            : lobby.phase == gloom::gameplay::SliceMatchPhase::active
                                  ? "active"
                                  : "completed";
    std::cout << "Lobby " << phase << " revision " << lobby.revision << ':';
    for (const auto& player : lobby.players) {
        std::cout << ' ' << player.identity.display_name << '='
                  << (player.connected ? "connected" : "disconnected") << '/'
                  << (player.ready ? "ready" : "not-ready")
                  << '/'
                  << (player.selection.character == gloom::gameplay::SliceCharacter::archangel ? "Archangel" :
                      player.selection.character == gloom::gameplay::SliceCharacter::shadow ? "Shadow" : "Hound")
                  << "/Soul-Reaper/"
                  << (player.selection.ability == gloom::gameplay::SliceAbility::bite
                          ? "Bite"
                          : player.selection.ability == gloom::gameplay::SliceAbility::guard
                                ? "Guard"
                                : "None");
    }
    if (lobby.phase == gloom::gameplay::SliceMatchPhase::completed) {
        std::cout << " winner=" << lobby.winner_entity << " reason=abandonment";
    }
    std::cout << '\n';
}

[[nodiscard]] std::uint64_t parse_tick_limit(const int argument_count,
                                             const char* const* arguments) {
    if (argument_count == 2) {
        return 0;
    }
    if (argument_count != 4 || std::string_view{arguments[2]} != "--ticks") {
        throw std::invalid_argument{
            "Usage: gloom_slice_server IP:port [--ticks count]"};
    }
    std::uint64_t ticks = 0;
    const std::string_view text{arguments[3]};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), ticks);
    if (error != std::errc{} || end != text.data() + text.size() || ticks == 0) {
        throw std::invalid_argument{"Server tick count must be a positive integer"};
    }
    return ticks;
}

} // namespace

int main(const int argument_count, const char* const* arguments) try {
    if (argument_count < 2) {
        throw std::invalid_argument{
            "Usage: gloom_slice_server IP:port [--ticks count]"};
    }
    const std::uint64_t tick_limit = parse_tick_limit(argument_count, arguments);
    std::signal(SIGINT, stop_server);
    std::signal(SIGTERM, stop_server);

    gloom::backends::GnsTransport transport;
    gloom::backends::JoltWorld authoritative_physics;
    authoritative_physics.start();
    const auto instance = gloom::backends::secure_server_instance();
    std::shared_ptr<const gloom::gameplay::SliceIdentityProvider> identity;
    if (const auto* mode = std::getenv("GLOOM_GAME_AUTH"); mode && *mode) {
        if (std::string_view{mode} != "tickets") throw std::invalid_argument{"Unknown GLOOM_GAME_AUTH mode"};
        const auto* key = std::getenv("GLOOM_GAME_TICKET_PUBLIC_KEY");
        if (!key || !std::getenv("GLOOM_MATCH_SERVICE_URL"))
            throw std::invalid_argument{"Ticket mode requires public key and match service URL"};
        identity = gloom::backends::make_game_ticket_verifier(key, instance, monotonic_milliseconds);
    }
    gloom::gameplay::VerticalSliceRemoteHost host{
        gloom::gameplay::SliceRemoteHostSettings{
            .identity_provider = std::move(identity),
            .authoritative_physics = &authoritative_physics,
            .generate_resume_token = gloom::backends::secure_session_token, .original_factory = true}};
    transport.start();
    const auto endpoint = transport.listen(arguments[1]);
    std::unique_ptr<gloom::gameplay::SliceMatchDirectory> directory;
    if (const char* url = std::getenv("GLOOM_MATCH_SERVICE_URL"); url != nullptr) {
        const char* token = std::getenv("GLOOM_MATCH_PUBLISHER_TOKEN");
        if (token == nullptr) throw std::runtime_error{"GLOOM_MATCH_PUBLISHER_TOKEN is required"};
        auto connector = gloom::backends::make_winhttp_match_service_connector();
        auto connected = gloom::gameplay::connect_match_directory(
            *connector, {.service_url = url, .bearer_token = token,
                         .wait = [](const std::uint64_t delay) {
                             std::this_thread::sleep_for(std::chrono::milliseconds{delay});
                         }});
        if (!connected) throw std::runtime_error{connected.error()};
        directory = std::move(*connected);
    } else {
        directory = gloom::gameplay::make_in_memory_match_directory();
    }
    gloom::gameplay::SliceMatchPublication publication{
        *directory,
        {.match_id = "local-factory-duel",
         .instance_id = instance,
         .display_name = "Factory duel",
         .endpoint = endpoint}};
    if (auto published = publication.start(host.lobby(), monotonic_milliseconds());
        !published) {
        throw std::runtime_error{"Could not publish slice match: " + published.error()};
    }
    std::cout << "Gloom dedicated slice server listening on " << endpoint
              << "; match local-factory-duel published; Ctrl+C stops it.\n";

    constexpr auto tick_duration = std::chrono::duration<double>{1.0 / 60.0};
    auto next_tick = std::chrono::steady_clock::now();
    std::uint64_t ticks = 0;
    std::uint64_t lobby_revision = 0;
    while (running.load(std::memory_order_relaxed) &&
           (tick_limit == 0 || ticks < tick_limit)) {
        transport.tick(tick_duration.count());
        const double now = std::chrono::duration<double>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();
        while (auto event = transport.poll_event()) {
            if (!event->incoming) {
                continue;
            }
            if (event->state == gloom::network::ConnectionState::connected) {
                host.connected(event->connection);
            } else if (event->state == gloom::network::ConnectionState::disconnected ||
                       event->state == gloom::network::ConnectionState::failed) {
                host.disconnected(event->connection);
            }
        }
        while (auto packet = transport.receive()) {
            const auto message = gloom::network::decode_message(packet->payload);
            if (!message) {
                continue;
            }
            for (const auto& response : host.receive(packet->connection, *message, now)) {
                send(transport, response);
            }
        }
        for (const auto& snapshot : host.tick_clients()) {
            send(transport, snapshot);
        }
        if (auto published = publication.update(host.lobby(), monotonic_milliseconds());
            !published) {
            throw std::runtime_error{"Could not refresh slice match: " + published.error()};
        }
        if (host.lobby().revision > lobby_revision) {
            lobby_revision = host.lobby().revision;
            print_lobby(host.lobby());
        }
        ++ticks;
        next_tick += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            tick_duration);
        std::this_thread::sleep_until(next_tick);
    }

    const auto link = transport.metrics();
    const auto& sessions = host.session_metrics();
    static_cast<void>(publication.stop());
    transport.stop();
    // The host and its body-owning components outlive this point and are
    // destroyed before the world by declaration order.
    std::cout << "Dedicated slice server stopped after " << ticks << " ticks; "
              << sessions.admitted << " admitted, " << sessions.resumed << " resumed, "
              << sessions.disconnected << " disconnected; " << link.sent_packets
              << " packets sent and " << link.received_packets << " received.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Gloom dedicated server failed: " << error.what() << '\n';
    return 1;
}
