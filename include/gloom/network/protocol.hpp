#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace gloom::network {

inline constexpr std::uint16_t protocol_version = 14;
inline constexpr std::size_t protocol_header_size = 28;
inline constexpr std::size_t maximum_protocol_payload = 1024 * 1024;

enum class MessageKind : std::uint8_t {
    client_hello = 1,
    server_welcome = 2,
    input_command = 3,
    snapshot = 4,
    event = 5,
    disconnect = 6,
    clock_request = 7,
    clock_response = 8,
    event_acknowledgement = 9,
    gameplay_snapshot = 10,
    lobby_command = 11,
    lobby_state = 12,
    ability_command = 13,
};

enum class MessageFlags : std::uint8_t {
    none = 0,
    full_snapshot = 1 << 0,
    delta_snapshot = 1 << 1,
};

struct ProtocolMessage {
    MessageKind kind{MessageKind::client_hello};
    MessageFlags flags{MessageFlags::none};
    std::uint64_t simulation_tick{0};
    std::uint32_t sequence{0};
    std::uint32_t acknowledged_sequence{0};
    std::vector<std::byte> payload;
};

[[nodiscard]] std::vector<std::byte> encode_message(const ProtocolMessage& message);
[[nodiscard]] std::expected<ProtocolMessage, std::string>
decode_message(std::span<const std::byte> encoded,
               std::size_t maximum_payload = maximum_protocol_payload);

// Wrap-aware comparison for monotonically increasing 32-bit packet/input sequences.
// It is only meaningful when the compared values are less than 2^31 apart.
[[nodiscard]] constexpr bool sequence_more_recent(const std::uint32_t candidate,
                                                  const std::uint32_t reference) noexcept {
    return candidate != reference &&
           static_cast<std::int32_t>(candidate - reference) > 0;
}

} // namespace gloom::network
