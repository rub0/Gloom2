#pragma once

#include <gloom/network/clock_sync.hpp>
#include <gloom/network/combat.hpp>
#include <gloom/network/protocol.hpp>
#include <gloom/network/transport.hpp>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <expected>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gloom::network {

using SessionId = std::uint64_t;
inline constexpr SessionId invalid_session = 0;

struct ClientHello {
    std::uint64_t client_nonce{0};
    std::uint64_t resume_token{0};
    std::string credential;
};

struct ServerWelcome {
    std::uint64_t client_nonce{0};
    SessionId session{invalid_session};
    NetworkEntityId controlled_entity{0};
    std::uint64_t resume_token{0};
    std::uint64_t server_tick{0};
    double server_time_seconds{0.0};
};

struct ClockRequest {
    std::uint64_t nonce{0};
    double client_send_seconds{0.0};
};

struct ClockResponse {
    std::uint64_t nonce{0};
    double client_send_seconds{0.0};
    double server_receive_seconds{0.0};
    double server_send_seconds{0.0};
};

[[nodiscard]] ProtocolMessage encode_client_hello(const ClientHello& hello);
[[nodiscard]] std::expected<ClientHello, std::string>
decode_client_hello(const ProtocolMessage& message);
[[nodiscard]] ProtocolMessage encode_server_welcome(const ServerWelcome& welcome);
[[nodiscard]] std::expected<ServerWelcome, std::string>
decode_server_welcome(const ProtocolMessage& message);
[[nodiscard]] ProtocolMessage encode_clock_request(const ClockRequest& request);
[[nodiscard]] std::expected<ClockRequest, std::string>
decode_clock_request(const ProtocolMessage& message);
[[nodiscard]] ProtocolMessage encode_clock_response(const ClockResponse& response);
[[nodiscard]] std::expected<ClockResponse, std::string>
decode_clock_response(const ProtocolMessage& message);

struct SessionReplicationState {
    std::uint32_t acknowledged_snapshot{0};
    bool has_acknowledged_snapshot{false};
    std::uint64_t received_input_batches{0};
};

struct SessionSettings {
    std::size_t maximum_clients{64};
    std::uint64_t reconnect_grace_ticks{600};
    std::size_t maximum_credential_bytes{256};
    std::function<bool(std::string_view)> authenticate;
    std::function<std::uint64_t()> generate_resume_token;
};

struct SessionMetrics {
    std::uint64_t admitted{0};
    std::uint64_t resumed{0};
    std::uint64_t rejected_authentication{0};
    std::uint64_t rejected_capacity{0};
    std::uint64_t rejected_protocol{0};
    std::uint64_t disconnected{0};
    std::uint64_t expired{0};
    std::uint64_t unauthorized_messages{0};
};

class ServerSessionManager final {
public:
    explicit ServerSessionManager(SessionSettings settings);

    void connected(ConnectionId connection);
    [[nodiscard]] std::expected<ServerWelcome, std::string>
    admit(ConnectionId connection,
          const ClientHello& hello,
          std::uint64_t server_tick,
          double server_time_seconds, std::string authenticated_identity = {});
    [[nodiscard]] bool pending(ConnectionId connection) const noexcept;
    void disconnected(ConnectionId connection, std::uint64_t server_tick);
    void expire(std::uint64_t server_tick);

    [[nodiscard]] std::expected<NetworkEntityId, std::string>
    authorize_input(ConnectionId connection, const ProtocolMessage& message);
    [[nodiscard]] bool authorize_fire(ConnectionId connection, const FireCommand& command);
    [[nodiscard]] std::optional<NetworkEntityId>
    controlled_entity(ConnectionId connection) const noexcept;
    [[nodiscard]] const SessionReplicationState*
    replication_state(ConnectionId connection) const noexcept;
    [[nodiscard]] std::size_t active_sessions() const noexcept;
    [[nodiscard]] std::size_t dormant_sessions() const noexcept;
    [[nodiscard]] const SessionMetrics& metrics() const noexcept;

private:
    enum class State { active, dormant };
    struct Record {
        SessionId session{invalid_session};
        ConnectionId connection{invalid_connection};
        NetworkEntityId controlled_entity{0};
        std::uint64_t resume_token{0};
        std::uint64_t expires_at_tick{0};
        State state{State::active};
        SessionReplicationState replication;
        std::string authenticated_identity;
    };

    [[nodiscard]] Record* find_active(ConnectionId connection) noexcept;
    [[nodiscard]] const Record* find_active(ConnectionId connection) const noexcept;
    [[nodiscard]] std::uint64_t issue_resume_token();

    SessionSettings settings_;
    std::unordered_set<ConnectionId> pending_connections_;
    std::unordered_map<SessionId, Record> sessions_;
    SessionId next_session_{1};
    NetworkEntityId next_entity_{1};
    SessionMetrics metrics_;
};

class ClientSession final {
public:
    [[nodiscard]] ProtocolMessage begin(std::string credential, std::uint64_t client_nonce);
    [[nodiscard]] ProtocolMessage reconnect(std::string credential,
                                            std::uint64_t client_nonce);
    void accept(const ProtocolMessage& message);
    [[nodiscard]] ProtocolMessage create_clock_request(double client_time_seconds);
    void receive_clock_response(const ProtocolMessage& message,
                                double client_receive_seconds);

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] SessionId session() const noexcept;
    [[nodiscard]] NetworkEntityId controlled_entity() const noexcept;
    [[nodiscard]] std::uint64_t resume_token() const noexcept;
    [[nodiscard]] const ClockSynchronizer& clock() const noexcept;

private:
    SessionId session_{invalid_session};
    NetworkEntityId controlled_entity_{0};
    std::uint64_t resume_token_{0};
    std::uint64_t expected_client_nonce_{0};
    std::uint64_t next_clock_nonce_{1};
    std::unordered_map<std::uint64_t, double> pending_clock_requests_;
    ClockSynchronizer clock_;
};

struct ReliableEventSettings {
    double retry_interval_seconds{0.1};
    double lifetime_seconds{2.0};
    std::size_t maximum_pending{128};
    std::size_t duplicate_window{1024};
};

struct ReliableEventMetrics {
    std::uint64_t queued{0};
    std::uint64_t transmissions{0};
    std::uint64_t retransmissions{0};
    std::uint64_t acknowledged{0};
    std::uint64_t expired{0};
    std::uint64_t delivered{0};
    std::uint64_t duplicates{0};
};

struct ReceivedSessionEvent {
    std::uint32_t sequence{0};
    bool duplicate{false};
    std::vector<std::byte> payload;
};

class ReliableEventSender final {
public:
    explicit ReliableEventSender(ReliableEventSettings settings = {});

    [[nodiscard]] std::uint32_t queue(std::span<const std::byte> payload,
                                      double now_seconds);
    [[nodiscard]] std::vector<ProtocolMessage> poll(double now_seconds);
    [[nodiscard]] bool acknowledge(const ProtocolMessage& message);
    [[nodiscard]] std::size_t pending() const noexcept;
    [[nodiscard]] const ReliableEventMetrics& metrics() const noexcept;

private:
    struct PendingEvent {
        std::uint32_t sequence{0};
        std::vector<std::byte> payload;
        double queued_seconds{0.0};
        double last_send_seconds{0.0};
        std::uint32_t transmissions{0};
    };

    ReliableEventSettings settings_;
    std::deque<PendingEvent> pending_;
    std::uint32_t next_sequence_{1};
    ReliableEventMetrics metrics_;
};

class ReliableEventReceiver final {
public:
    explicit ReliableEventReceiver(std::size_t duplicate_window = 1024);

    [[nodiscard]] std::expected<ReceivedSessionEvent, std::string>
    receive(const ProtocolMessage& message);
    [[nodiscard]] ProtocolMessage acknowledgement(std::uint32_t sequence) const;
    [[nodiscard]] const ReliableEventMetrics& metrics() const noexcept;

private:
    std::size_t duplicate_window_;
    std::deque<std::uint32_t> received_order_;
    std::unordered_set<std::uint32_t> received_;
    ReliableEventMetrics metrics_;
};

} // namespace gloom::network
