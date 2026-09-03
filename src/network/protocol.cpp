#include <gloom/network/protocol.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace gloom::network {
namespace {

constexpr std::array magic{static_cast<std::byte>('G'),
                           static_cast<std::byte>('L'),
                           static_cast<std::byte>('O'),
                           static_cast<std::byte>('M')};

template <typename Integer>
void append_little_endian(std::vector<std::byte>& output, const Integer value) {
    static_assert(std::is_unsigned_v<Integer>);
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        output.push_back(static_cast<std::byte>(value >> (index * 8)));
    }
}

template <typename Integer>
[[nodiscard]] Integer read_little_endian(const std::span<const std::byte> input,
                                         std::size_t& offset) {
    static_assert(std::is_unsigned_v<Integer>);
    Integer value = 0;
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        value |= static_cast<Integer>(std::to_integer<unsigned int>(input[offset++]))
                 << (index * 8);
    }
    return value;
}

[[nodiscard]] bool valid_kind(const MessageKind kind) noexcept {
    switch (kind) {
        case MessageKind::client_hello:
        case MessageKind::server_welcome:
        case MessageKind::input_command:
        case MessageKind::snapshot:
        case MessageKind::event:
        case MessageKind::disconnect:
        case MessageKind::clock_request:
        case MessageKind::clock_response:
        case MessageKind::event_acknowledgement:
        case MessageKind::gameplay_snapshot:
        case MessageKind::lobby_command:
        case MessageKind::lobby_state:
        case MessageKind::ability_command:
            return true;
    }
    return false;
}

} // namespace

std::vector<std::byte> encode_message(const ProtocolMessage& message) {
    if (!valid_kind(message.kind)) {
        throw std::invalid_argument{"Cannot encode an unknown protocol message kind"};
    }
    if (message.payload.size() > maximum_protocol_payload ||
        message.payload.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error{"Protocol message payload exceeds the configured maximum"};
    }

    std::vector<std::byte> output;
    output.reserve(protocol_header_size + message.payload.size());
    output.insert(output.end(), magic.begin(), magic.end());
    append_little_endian(output, protocol_version);
    output.push_back(static_cast<std::byte>(message.kind));
    output.push_back(static_cast<std::byte>(message.flags));
    append_little_endian(output, message.simulation_tick);
    append_little_endian(output, message.sequence);
    append_little_endian(output, message.acknowledged_sequence);
    append_little_endian(output, static_cast<std::uint32_t>(message.payload.size()));
    output.insert(output.end(), message.payload.begin(), message.payload.end());
    return output;
}

std::expected<ProtocolMessage, std::string>
decode_message(const std::span<const std::byte> encoded, const std::size_t maximum_payload) {
    if (encoded.size() < protocol_header_size) {
        return std::unexpected{"Protocol message is shorter than its header"};
    }
    if (!std::equal(magic.begin(), magic.end(), encoded.begin())) {
        return std::unexpected{"Protocol magic does not match Gloom"};
    }

    std::size_t offset = magic.size();
    const auto version = read_little_endian<std::uint16_t>(encoded, offset);
    if (version != protocol_version) {
        return std::unexpected{"Unsupported Gloom protocol version"};
    }

    ProtocolMessage message;
    message.kind = static_cast<MessageKind>(std::to_integer<std::uint8_t>(encoded[offset++]));
    if (!valid_kind(message.kind)) {
        return std::unexpected{"Unknown protocol message kind"};
    }
    message.flags = static_cast<MessageFlags>(std::to_integer<std::uint8_t>(encoded[offset++]));
    message.simulation_tick = read_little_endian<std::uint64_t>(encoded, offset);
    message.sequence = read_little_endian<std::uint32_t>(encoded, offset);
    message.acknowledged_sequence = read_little_endian<std::uint32_t>(encoded, offset);
    const auto payload_size = read_little_endian<std::uint32_t>(encoded, offset);
    if (payload_size > maximum_payload) {
        return std::unexpected{"Protocol payload exceeds the configured maximum"};
    }
    if (encoded.size() != protocol_header_size + static_cast<std::size_t>(payload_size)) {
        return std::unexpected{"Protocol payload size does not match the datagram"};
    }
    message.payload.assign(encoded.begin() + static_cast<std::ptrdiff_t>(offset), encoded.end());
    return message;
}

} // namespace gloom::network
