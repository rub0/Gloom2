#pragma once

#include <gloom/core/subsystem.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gloom::network {

using ConnectionId = std::uint64_t;
inline constexpr ConnectionId invalid_connection = 0;

enum class Delivery {
    unreliable,
    reliable,
};

struct PacketView {
    std::span<const std::byte> payload;
    Delivery delivery{Delivery::unreliable};
};

enum class ConnectionState {
    connecting,
    connected,
    disconnected,
    failed,
};

struct ConnectionEvent {
    ConnectionId connection{invalid_connection};
    ConnectionState state{ConnectionState::connecting};
    bool incoming{false};
    std::int32_t reason{0};
    std::string description;
};

struct ReceivedPacket {
    ConnectionId connection{invalid_connection};
    std::vector<std::byte> payload;
    Delivery delivery{Delivery::unreliable};
    std::int64_t sequence{0};
};

struct TransportMetrics {
    std::uint64_t opened_connections{0};
    std::uint64_t sent_packets{0};
    std::uint64_t received_packets{0};
    std::uint64_t sent_bytes{0};
    std::uint64_t received_bytes{0};
};

// Backend-independent, message-oriented transport contract. Calls, including tick(),
// are serialized by the owning engine thread in the current implementation.
class Transport : public core::Subsystem {
public:
    [[nodiscard]] virtual std::string listen(std::string_view endpoint) = 0;
    virtual void stop_listening() = 0;
    [[nodiscard]] virtual ConnectionId connect(std::string_view endpoint) = 0;
    virtual void disconnect(ConnectionId connection) = 0;
    virtual void send(ConnectionId connection, PacketView packet) = 0;
    [[nodiscard]] virtual std::optional<ConnectionEvent> poll_event() = 0;
    [[nodiscard]] virtual std::optional<ReceivedPacket> receive() = 0;
    [[nodiscard]] virtual TransportMetrics metrics() const noexcept = 0;
};

} // namespace gloom::network
