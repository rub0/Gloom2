#include <gloom/network/movement_replication.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>

namespace gloom::network {
namespace {

constexpr std::size_t input_command_size = 21;
constexpr std::size_t input_batch_header_size = 1;
constexpr std::size_t snapshot_header_size = 6;
constexpr std::size_t snapshot_record_header_size = 10;
constexpr std::size_t maximum_snapshot_entities = 4096;
constexpr float position_quantization = 256.0F;
constexpr float velocity_quantization = 128.0F;
constexpr std::uint8_t removed_record = 1;
constexpr std::uint8_t position_x_changed = 1 << 0;
constexpr std::uint8_t position_y_changed = 1 << 1;
constexpr std::uint8_t position_z_changed = 1 << 2;
constexpr std::uint8_t velocity_x_changed = 1 << 3;
constexpr std::uint8_t velocity_y_changed = 1 << 4;
constexpr std::uint8_t velocity_z_changed = 1 << 5;
constexpr std::uint8_t grounded_changed = 1 << 6;
constexpr std::uint8_t all_state_fields = (1 << 7) - 1;

struct QuantizedMovementState {
    std::int32_t position_x{0};
    std::int32_t position_y{0};
    std::int32_t position_z{0};
    std::int16_t velocity_x{0};
    std::int16_t velocity_y{0};
    std::int16_t velocity_z{0};
    bool grounded{false};
    bool air_dodge_available{false};
};

template <typename Integer>
void append_integer(std::vector<std::byte>& output, const Integer value) {
    static_assert(std::is_unsigned_v<Integer>);
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        output.push_back(static_cast<std::byte>(value >> (index * 8)));
    }
}

void append_float(std::vector<std::byte>& output, const float value) {
    append_integer(output, std::bit_cast<std::uint32_t>(value));
}

template <typename Signed>
void append_signed(std::vector<std::byte>& output, const Signed value) {
    using Unsigned = std::make_unsigned_t<Signed>;
    append_integer(output, std::bit_cast<Unsigned>(value));
}

template <typename Integer>
[[nodiscard]] Integer read_integer(const std::span<const std::byte> input, std::size_t& offset) {
    static_assert(std::is_unsigned_v<Integer>);
    Integer value = 0;
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        value |= static_cast<Integer>(std::to_integer<unsigned int>(input[offset++]))
                 << (index * 8);
    }
    return value;
}

[[nodiscard]] float read_float(const std::span<const std::byte> input, std::size_t& offset) {
    return std::bit_cast<float>(read_integer<std::uint32_t>(input, offset));
}

template <typename Signed>
[[nodiscard]] Signed read_signed(const std::span<const std::byte> input, std::size_t& offset) {
    using Unsigned = std::make_unsigned_t<Signed>;
    return std::bit_cast<Signed>(read_integer<Unsigned>(input, offset));
}

[[nodiscard]] QuantizedMovementState quantize_state(const MovementState& state) {
    const auto quantize_position = [](const float value) {
        const double scaled = std::round(static_cast<double>(value) * position_quantization);
        if (scaled < std::numeric_limits<std::int32_t>::min() ||
            scaled > std::numeric_limits<std::int32_t>::max()) {
            throw std::invalid_argument{"Snapshot position exceeds quantized range"};
        }
        return static_cast<std::int32_t>(scaled);
    };
    const auto quantize_velocity = [](const float value) {
        const double scaled = std::round(static_cast<double>(value) * velocity_quantization);
        if (scaled < std::numeric_limits<std::int16_t>::min() ||
            scaled > std::numeric_limits<std::int16_t>::max()) {
            throw std::invalid_argument{"Snapshot velocity exceeds quantized range"};
        }
        return static_cast<std::int16_t>(scaled);
    };
    return {
        .position_x = quantize_position(state.position_x),
        .position_y = quantize_position(state.position_y),
        .position_z = quantize_position(state.position_z),
        .velocity_x = quantize_velocity(state.velocity_x),
        .velocity_y = quantize_velocity(state.velocity_y),
        .velocity_z = quantize_velocity(state.velocity_z),
        .grounded = state.grounded,
        .air_dodge_available = state.air_dodge_available,
    };
}

[[nodiscard]] MovementState dequantize_state(const NetworkEntityId entity,
                                             const std::uint64_t simulation_tick,
                                             const QuantizedMovementState& state) noexcept {
    return {
        .entity = entity,
        .simulation_tick = simulation_tick,
        .position_x = static_cast<float>(state.position_x) / position_quantization,
        .position_y = static_cast<float>(state.position_y) / position_quantization,
        .position_z = static_cast<float>(state.position_z) / position_quantization,
        .velocity_x = static_cast<float>(state.velocity_x) / velocity_quantization,
        .velocity_y = static_cast<float>(state.velocity_y) / velocity_quantization,
        .velocity_z = static_cast<float>(state.velocity_z) / velocity_quantization,
        .grounded = state.grounded,
        .air_dodge_available = state.air_dodge_available,
    };
}

[[nodiscard]] std::uint8_t changed_fields(const QuantizedMovementState& current,
                                          const QuantizedMovementState& baseline) noexcept {
    std::uint8_t mask = 0;
    mask |= current.position_x != baseline.position_x ? position_x_changed : 0;
    mask |= current.position_y != baseline.position_y ? position_y_changed : 0;
    mask |= current.position_z != baseline.position_z ? position_z_changed : 0;
    mask |= current.velocity_x != baseline.velocity_x ? velocity_x_changed : 0;
    mask |= current.velocity_y != baseline.velocity_y ? velocity_y_changed : 0;
    mask |= current.velocity_z != baseline.velocity_z ? velocity_z_changed : 0;
    mask |= (current.grounded != baseline.grounded || current.air_dodge_available != baseline.air_dodge_available) ? grounded_changed : 0;
    return mask;
}

[[nodiscard]] std::size_t encoded_fields_size(const std::uint8_t mask) noexcept {
    std::size_t size = 0;
    size += (mask & position_x_changed) != 0 ? sizeof(std::int32_t) : 0;
    size += (mask & position_y_changed) != 0 ? sizeof(std::int32_t) : 0;
    size += (mask & position_z_changed) != 0 ? sizeof(std::int32_t) : 0;
    size += (mask & velocity_x_changed) != 0 ? sizeof(std::int16_t) : 0;
    size += (mask & velocity_y_changed) != 0 ? sizeof(std::int16_t) : 0;
    size += (mask & velocity_z_changed) != 0 ? sizeof(std::int16_t) : 0;
    size += (mask & grounded_changed) != 0 ? sizeof(std::uint8_t) : 0;
    return size;
}

[[nodiscard]] bool finite_state(const MovementState& state) noexcept {
    return std::isfinite(state.position_x) && std::isfinite(state.position_y) &&
           std::isfinite(state.position_z) && std::isfinite(state.velocity_x) &&
           std::isfinite(state.velocity_y) && std::isfinite(state.velocity_z);
}

[[nodiscard]] bool valid_obstacle(const StaticMovementObstacle& obstacle) noexcept {
    return std::isfinite(obstacle.minimum_x) && std::isfinite(obstacle.maximum_x) &&
           std::isfinite(obstacle.minimum_z) && std::isfinite(obstacle.maximum_z) &&
           std::isfinite(obstacle.top_y) && obstacle.minimum_x < obstacle.maximum_x &&
           obstacle.minimum_z < obstacle.maximum_z && obstacle.top_y > 0.0F;
}

[[nodiscard]] bool overlaps_obstacle(const float x,
                                     const float z,
                                     const float radius,
                                     const StaticMovementObstacle& obstacle) noexcept {
    const float closest_x = std::clamp(x, obstacle.minimum_x, obstacle.maximum_x);
    const float closest_z = std::clamp(z, obstacle.minimum_z, obstacle.maximum_z);
    const float difference_x = x - closest_x;
    const float difference_z = z - closest_z;
    return difference_x * difference_x + difference_z * difference_z <= radius * radius;
}

[[nodiscard]] float support_height(const float x,
                                   const float z,
                                   const ReplicationSettings& settings) noexcept {
    float height = 0.0F;
    for (const auto& obstacle : settings.static_obstacles) {
        if (overlaps_obstacle(x, z, settings.character_radius, obstacle)) {
            height = std::max(height, obstacle.top_y);
        }
    }
    return height;
}

[[nodiscard]] bool horizontal_motion_blocked(const float x,
                                             const float z,
                                             const float feet_y,
                                             const ReplicationSettings& settings) noexcept {
    for (const auto& obstacle : settings.static_obstacles) {
        if (feet_y + 0.001F < obstacle.top_y &&
            overlaps_obstacle(x, z, settings.character_radius, obstacle)) {
            return true;
        }
    }
    return false;
}

void validate_settings(const ReplicationSettings& settings) {
    if (settings.tick_rate == 0 || settings.snapshot_rate == 0 ||
        settings.tick_rate % settings.snapshot_rate != 0) {
        throw std::invalid_argument{
            "Replication tick rate must be non-zero and divisible by snapshot rate"};
    }
    if (!std::isfinite(settings.movement_speed) || settings.movement_speed < 0.0F ||
        !std::isfinite(settings.interpolation_delay_seconds) ||
        settings.interpolation_delay_seconds < 0.0 || !std::isfinite(settings.jump_speed) ||
        settings.jump_speed <= 0.0F || !std::isfinite(settings.gravity) ||
        settings.gravity >= 0.0F || !std::isfinite(settings.character_radius) ||
        settings.character_radius <= 0.0F ||
        !std::isfinite(settings.character_collision_height) ||
        settings.character_collision_height <= 0.0F || settings.input_redundancy == 0 ||
        settings.input_redundancy > maximum_movement_input_batch ||
        !std::isfinite(settings.relevance_radius) || settings.relevance_radius < 0.0F ||
        settings.maximum_snapshot_entities == 0 ||
        settings.maximum_snapshot_entities > maximum_snapshot_entities ||
        !std::ranges::all_of(settings.static_obstacles, valid_obstacle)) {
        throw std::invalid_argument{"Replication movement and interpolation settings are invalid"};
    }
}

[[nodiscard]] MovementState interpolate_state(const MovementState& left,
                                              const MovementState& right,
                                              const double alpha) {
    const auto blend = [alpha](const float from, const float to) {
        return static_cast<float>(from + (to - from) * alpha);
    };
    MovementState state = left;
    state.simulation_tick = alpha < 0.5 ? left.simulation_tick : right.simulation_tick;
    state.position_x = blend(left.position_x, right.position_x);
    state.position_y = blend(left.position_y, right.position_y);
    state.position_z = blend(left.position_z, right.position_z);
    state.velocity_x = blend(left.velocity_x, right.velocity_x);
    state.velocity_y = blend(left.velocity_y, right.velocity_y);
    state.velocity_z = blend(left.velocity_z, right.velocity_z);
    state.grounded = alpha < 0.5 ? left.grounded : right.grounded;
    state.air_dodge_available = alpha < 0.5 ? left.air_dodge_available : right.air_dodge_available;
    return state;
}

[[nodiscard]] MovementState simulate_movement_unchecked(
    MovementState state,
    const MovementInput& input,
    const double fixed_delta_seconds,
    const ReplicationSettings& settings) {
    if (settings.scene_movement) return settings.scene_movement(state,input,fixed_delta_seconds);
    float axis_x = input.axis_x;
    float axis_z = input.axis_z;
    const float length_squared = axis_x * axis_x + axis_z * axis_z;
    if (length_squared > 1.0F) {
        const float inverse_length = 1.0F / std::sqrt(length_squared);
        axis_x *= inverse_length;
        axis_z *= inverse_length;
    }
    state.simulation_tick = input.simulation_tick;
    state.velocity_x = axis_x * settings.movement_speed;
    state.velocity_z = axis_z * settings.movement_speed;
    const float delta = static_cast<float>(fixed_delta_seconds);

    const float candidate_x = state.position_x + state.velocity_x * delta;
    if (!horizontal_motion_blocked(candidate_x, state.position_z, state.position_y, settings)) {
        state.position_x = candidate_x;
    } else {
        state.velocity_x = 0.0F;
    }
    const float candidate_z = state.position_z + state.velocity_z * delta;
    if (!horizontal_motion_blocked(state.position_x, candidate_z, state.position_y, settings)) {
        state.position_z = candidate_z;
    } else {
        state.velocity_z = 0.0F;
    }

    const float support = support_height(state.position_x, state.position_z, settings);
    if (state.grounded && std::abs(state.position_y - support) > 0.001F) {
        state.grounded = false;
    }
    if (input.jump && state.grounded) {
        state.velocity_y = settings.jump_speed;
        state.grounded = false;
    }
    if (!state.grounded) {
        state.velocity_y += settings.gravity * delta;
        const float candidate_y = state.position_y + state.velocity_y * delta;
        if (state.velocity_y <= 0.0F && candidate_y <= support) {
            state.position_y = support;
            state.velocity_y = 0.0F;
            state.grounded = true;
        } else {
            state.position_y = candidate_y;
        }
    } else {
        state.position_y = support;
        state.velocity_y = 0.0F;
    }
    return state;
}

} // namespace

ProtocolMessage encode_movement_input_batch(const std::span<const MovementInput> inputs) {
    if (inputs.empty() || inputs.size() > maximum_movement_input_batch) {
        throw std::invalid_argument{"Movement input batch size is invalid"};
    }
    ProtocolMessage message;
    message.kind = MessageKind::input_command;
    message.simulation_tick = inputs.back().simulation_tick;
    message.sequence = inputs.back().sequence;
    message.payload.reserve(input_batch_header_size + inputs.size() * input_command_size);
    append_integer(message.payload, static_cast<std::uint8_t>(inputs.size()));
    std::optional<std::uint32_t> previous_sequence;
    for (const auto& input : inputs) {
        if (!std::isfinite(input.axis_x) || !std::isfinite(input.axis_z) ||
            std::abs(input.axis_x) > 1.0F || std::abs(input.axis_z) > 1.0F ||
            (previous_sequence && !sequence_more_recent(input.sequence, *previous_sequence))) {
            throw std::invalid_argument{
                "Movement input batch contains invalid or unordered commands"};
        }
        append_integer(message.payload, input.sequence);
        append_integer(message.payload, input.simulation_tick);
        append_float(message.payload, input.axis_x);
        append_float(message.payload, input.axis_z);
        append_integer(message.payload, static_cast<std::uint8_t>((input.jump?1:0)|(input.dodge?2:0)));
        previous_sequence = input.sequence;
    }
    return message;
}

std::expected<std::vector<MovementInput>, std::string>
decode_movement_input_batch(const ProtocolMessage& message) {
    if (message.kind != MessageKind::input_command ||
        message.payload.size() < input_batch_header_size) {
        return std::unexpected{"Message is not a valid movement input batch"};
    }
    std::size_t offset = 0;
    const std::uint8_t count = read_integer<std::uint8_t>(message.payload, offset);
    if (count == 0 || count > maximum_movement_input_batch ||
        message.payload.size() != input_batch_header_size +
                                      static_cast<std::size_t>(count) * input_command_size) {
        return std::unexpected{"Movement input batch count does not match its payload"};
    }
    std::vector<MovementInput> inputs;
    inputs.reserve(count);
    for (std::uint8_t index = 0; index < count; ++index) {
        MovementInput input;
        input.sequence = read_integer<std::uint32_t>(message.payload, offset);
        input.simulation_tick = read_integer<std::uint64_t>(message.payload, offset);
        input.axis_x = read_float(message.payload, offset);
        input.axis_z = read_float(message.payload, offset);
        const std::uint8_t jump = read_integer<std::uint8_t>(message.payload, offset);
        input.jump = (jump & 1) != 0;
        input.dodge = (jump & 2) != 0;
        if (jump > 3 || !std::isfinite(input.axis_x) || !std::isfinite(input.axis_z) ||
            std::abs(input.axis_x) > 1.0F || std::abs(input.axis_z) > 1.0F ||
            (!inputs.empty() &&
             !sequence_more_recent(input.sequence, inputs.back().sequence))) {
            return std::unexpected{"Movement input batch contains invalid commands"};
        }
        inputs.push_back(input);
    }
    if (inputs.back().sequence != message.sequence ||
        inputs.back().simulation_tick != message.simulation_tick) {
        return std::unexpected{"Movement input batch envelope does not identify its newest command"};
    }
    return inputs;
}

ProtocolMessage encode_movement_input(const MovementInput& input) {
    return encode_movement_input_batch(std::span{&input, std::size_t{1}});
}

std::expected<MovementInput, std::string>
decode_movement_input(const ProtocolMessage& message) {
    auto inputs = decode_movement_input_batch(message);
    if (!inputs || inputs->size() != 1) {
        return std::unexpected{inputs ? "Movement input message contains a batch"
                                      : inputs.error()};
    }
    return inputs->front();
}

namespace {

struct EncodedSnapshotRecord {
    NetworkEntityId entity{0};
    std::uint8_t flags{0};
    std::uint8_t mask{0};
    QuantizedMovementState state;
};

[[nodiscard]] ProtocolMessage encode_snapshot_impl(const WorldSnapshot& snapshot,
                                                   const std::uint32_t sequence,
                                                   const WorldSnapshot* baseline,
                                                   const std::uint32_t baseline_sequence) {
    if (snapshot.entities.size() > maximum_snapshot_entities ||
        snapshot.entities.size() > std::numeric_limits<std::uint16_t>::max()) {
        throw std::length_error{"World snapshot contains too many entities"};
    }
    if ((baseline == nullptr) != (baseline_sequence == 0)) {
        throw std::invalid_argument{"Snapshot baseline and sequence must be provided together"};
    }

    std::unordered_map<NetworkEntityId, QuantizedMovementState> baseline_states;
    if (baseline != nullptr) {
        baseline_states.reserve(baseline->entities.size());
        for (const auto& entity : baseline->entities) {
            if (entity.entity == 0 || !finite_state(entity) ||
                !baseline_states.emplace(entity.entity, quantize_state(entity)).second) {
                throw std::invalid_argument{"Snapshot baseline contains invalid entities"};
            }
        }
    }

    std::vector<EncodedSnapshotRecord> records;
    records.reserve(snapshot.entities.size() + baseline_states.size());
    std::unordered_set<NetworkEntityId> current_ids;
    for (const auto& entity : snapshot.entities) {
        if (entity.entity == 0 || !finite_state(entity) ||
            !current_ids.insert(entity.entity).second) {
            throw std::invalid_argument{"World snapshot contains an invalid entity state"};
        }
        const auto quantized = quantize_state(entity);
        const auto previous = baseline_states.find(entity.entity);
        const std::uint8_t mask = previous == baseline_states.end()
                                      ? all_state_fields
                                      : changed_fields(quantized, previous->second);
        if (baseline == nullptr || mask != 0) {
            records.push_back({.entity = entity.entity, .mask = mask, .state = quantized});
        }
    }
    if (baseline != nullptr) {
        for (const auto& [entity, state] : baseline_states) {
            static_cast<void>(state);
            if (!current_ids.contains(entity)) {
                records.push_back({.entity = entity, .flags = removed_record});
            }
        }
    }
    if (records.size() > maximum_snapshot_entities ||
        records.size() > std::numeric_limits<std::uint16_t>::max()) {
        throw std::length_error{"Encoded snapshot contains too many records"};
    }
    std::ranges::sort(records, {}, &EncodedSnapshotRecord::entity);

    ProtocolMessage message;
    message.kind = MessageKind::snapshot;
    message.flags = baseline == nullptr ? MessageFlags::full_snapshot
                                        : MessageFlags::delta_snapshot;
    message.simulation_tick = snapshot.simulation_tick;
    message.sequence = sequence;
    message.acknowledged_sequence = snapshot.acknowledged_input;
    append_integer(message.payload, baseline_sequence);
    append_integer(message.payload, static_cast<std::uint16_t>(records.size()));
    for (const auto& record : records) {
        append_integer(message.payload, record.entity);
        append_integer(message.payload, record.flags);
        append_integer(message.payload, record.mask);
        if ((record.flags & removed_record) != 0) {
            continue;
        }
        if ((record.mask & position_x_changed) != 0) {
            append_signed(message.payload, record.state.position_x);
        }
        if ((record.mask & position_y_changed) != 0) {
            append_signed(message.payload, record.state.position_y);
        }
        if ((record.mask & position_z_changed) != 0) {
            append_signed(message.payload, record.state.position_z);
        }
        if ((record.mask & velocity_x_changed) != 0) {
            append_signed(message.payload, record.state.velocity_x);
        }
        if ((record.mask & velocity_y_changed) != 0) {
            append_signed(message.payload, record.state.velocity_y);
        }
        if ((record.mask & velocity_z_changed) != 0) {
            append_signed(message.payload, record.state.velocity_z);
        }
        if ((record.mask & grounded_changed) != 0) {
            append_integer(message.payload, static_cast<std::uint8_t>((record.state.grounded ? 1 : 0) | (record.state.air_dodge_available << 1)));
        }
    }
    return message;
}

} // namespace

ProtocolMessage encode_world_snapshot(const WorldSnapshot& snapshot,
                                      const std::uint32_t sequence) {
    return encode_snapshot_impl(snapshot, sequence, nullptr, 0);
}

ProtocolMessage encode_world_snapshot_delta(const WorldSnapshot& snapshot,
                                            const std::uint32_t sequence,
                                            const WorldSnapshot& baseline,
                                            const std::uint32_t baseline_sequence) {
    return encode_snapshot_impl(snapshot, sequence, &baseline, baseline_sequence);
}

std::expected<std::uint32_t, std::string>
world_snapshot_baseline_sequence(const ProtocolMessage& message) {
    if (message.kind != MessageKind::snapshot || message.payload.size() < snapshot_header_size) {
        return std::unexpected{"Message is not a valid world snapshot"};
    }
    std::size_t offset = 0;
    const std::uint32_t baseline_sequence =
        read_integer<std::uint32_t>(message.payload, offset);
    if ((message.flags == MessageFlags::full_snapshot && baseline_sequence != 0) ||
        (message.flags == MessageFlags::delta_snapshot && baseline_sequence == 0) ||
        (message.flags != MessageFlags::full_snapshot &&
         message.flags != MessageFlags::delta_snapshot)) {
        return std::unexpected{"Snapshot flags and baseline are inconsistent"};
    }
    return baseline_sequence;
}

std::expected<WorldSnapshot, std::string>
decode_world_snapshot(const ProtocolMessage& message, const WorldSnapshot* baseline) {
    const auto baseline_sequence = world_snapshot_baseline_sequence(message);
    if (!baseline_sequence) {
        return std::unexpected{baseline_sequence.error()};
    }
    if ((*baseline_sequence == 0 && baseline != nullptr) ||
        (*baseline_sequence != 0 && baseline == nullptr)) {
        return std::unexpected{"Required snapshot baseline was not supplied correctly"};
    }
    std::size_t offset = 0;
    static_cast<void>(read_integer<std::uint32_t>(message.payload, offset));
    const auto count = read_integer<std::uint16_t>(message.payload, offset);
    if (count > maximum_snapshot_entities) {
        return std::unexpected{"World snapshot contains too many records"};
    }

    std::unordered_map<NetworkEntityId, QuantizedMovementState> states;
    if (baseline != nullptr) {
        states.reserve(baseline->entities.size() + count);
        for (const auto& entity : baseline->entities) {
            if (!states.emplace(entity.entity, quantize_state(entity)).second) {
                return std::unexpected{"Supplied snapshot baseline contains duplicate entities"};
            }
        }
    }
    std::unordered_set<NetworkEntityId> record_ids;
    for (std::uint16_t index = 0; index < count; ++index) {
        if (message.payload.size() - offset < snapshot_record_header_size) {
            return std::unexpected{"Snapshot record header is truncated"};
        }
        const auto entity = read_integer<std::uint64_t>(message.payload, offset);
        const auto flags = read_integer<std::uint8_t>(message.payload, offset);
        const auto mask = read_integer<std::uint8_t>(message.payload, offset);
        if (entity == 0 || flags > removed_record || (mask & ~all_state_fields) != 0 ||
            !record_ids.insert(entity).second) {
            return std::unexpected{"Snapshot record header is invalid"};
        }
        if ((flags & removed_record) != 0) {
            if (mask != 0 || baseline == nullptr || states.erase(entity) != 1) {
                return std::unexpected{"Snapshot removal record is invalid"};
            }
            continue;
        }
        if (message.payload.size() - offset < encoded_fields_size(mask)) {
            return std::unexpected{"Snapshot record fields are truncated"};
        }
        auto found = states.find(entity);
        if (found == states.end()) {
            if (mask != all_state_fields) {
                return std::unexpected{"New snapshot entity does not contain all fields"};
            }
            found = states.emplace(entity, QuantizedMovementState{}).first;
        } else if (baseline == nullptr) {
            return std::unexpected{"Full snapshot contains duplicate entity state"};
        }
        auto& state = found->second;
        if ((mask & position_x_changed) != 0) {
            state.position_x = read_signed<std::int32_t>(message.payload, offset);
        }
        if ((mask & position_y_changed) != 0) {
            state.position_y = read_signed<std::int32_t>(message.payload, offset);
        }
        if ((mask & position_z_changed) != 0) {
            state.position_z = read_signed<std::int32_t>(message.payload, offset);
        }
        if ((mask & velocity_x_changed) != 0) {
            state.velocity_x = read_signed<std::int16_t>(message.payload, offset);
        }
        if ((mask & velocity_y_changed) != 0) {
            state.velocity_y = read_signed<std::int16_t>(message.payload, offset);
        }
        if ((mask & velocity_z_changed) != 0) {
            state.velocity_z = read_signed<std::int16_t>(message.payload, offset);
        }
        if ((mask & grounded_changed) != 0) {
            const auto grounded = read_integer<std::uint8_t>(message.payload, offset);
            if (grounded > 3) {
                return std::unexpected{"Snapshot grounded field is invalid"};
            }
            state.grounded = (grounded & 1) != 0;
            state.air_dodge_available = (grounded & 2) != 0;
        }
    }
    if (offset != message.payload.size()) {
        return std::unexpected{"Snapshot payload contains trailing bytes"};
    }

    WorldSnapshot snapshot{.simulation_tick = message.simulation_tick,
                           .acknowledged_input = message.acknowledged_sequence};
    snapshot.entities.reserve(states.size());
    for (const auto& [entity, state] : states) {
        snapshot.entities.push_back(dequantize_state(entity, message.simulation_tick, state));
    }
    std::ranges::sort(snapshot.entities, {}, &MovementState::entity);
    return snapshot;
}

MovementState simulate_movement(MovementState state,
                                const MovementInput& input,
                                const double fixed_delta_seconds,
                                const ReplicationSettings& settings) {
    if (!std::isfinite(fixed_delta_seconds) || fixed_delta_seconds <= 0.0 ||
        !finite_state(state)) {
        throw std::invalid_argument{"Movement simulation settings are invalid"};
    }
    validate_settings(settings);
    if (!std::isfinite(input.axis_x) || !std::isfinite(input.axis_z)) {
        throw std::invalid_argument{"Movement input axes must be finite"};
    }
    return simulate_movement_unchecked(state, input, fixed_delta_seconds, settings);
}

AuthoritativeMovementServer::AuthoritativeMovementServer(const ReplicationSettings settings,
                                                         const NetworkEntityId controlled_entity)
    : settings_{settings}, controlled_entity_{controlled_entity} {
    validate_settings(settings_);
    if (controlled_entity_ == 0) {
        throw std::invalid_argument{"Controlled network entity ID must be valid"};
    }
    clients_.try_emplace(controlled_entity_);
}

void AuthoritativeMovementServer::add_entity(MovementState state) {
    if (state.entity == 0 || !finite_state(state) || entities_.contains(state.entity)) {
        throw std::invalid_argument{"Cannot add invalid or duplicate network entity"};
    }
    state.simulation_tick = simulation_tick_;
    entities_.emplace(state.entity, state);
    entity_inputs_.emplace(state.entity, MovementInput{});
    entity_collision_enabled_.emplace(state.entity, true);
}

void AuthoritativeMovementServer::register_client(const NetworkEntityId controlled_entity) {
    if (controlled_entity == 0 || clients_.contains(controlled_entity)) {
        throw std::invalid_argument{"Cannot register invalid or duplicate network client"};
    }
    clients_.try_emplace(controlled_entity);
}

void AuthoritativeMovementServer::unregister_client(const NetworkEntityId controlled_entity) {
    if (controlled_entity == controlled_entity_ || clients_.erase(controlled_entity) == 0) {
        throw std::invalid_argument{"Network client is not registered"};
    }
}

void AuthoritativeMovementServer::set_entity_input(const NetworkEntityId entity,
                                                   const float axis_x,
                                                   const float axis_z) {
    if (!entities_.contains(entity) || !std::isfinite(axis_x) || !std::isfinite(axis_z) ||
        std::abs(axis_x) > 1.0F || std::abs(axis_z) > 1.0F) {
        throw std::invalid_argument{"Invalid authoritative entity input"};
    }
    entity_inputs_[entity].axis_x = axis_x;
    entity_inputs_[entity].axis_z = axis_z;
}

void AuthoritativeMovementServer::teleport_entity(const NetworkEntityId entity,
                                                   const float position_x,
                                                   const float position_y,
                                                   const float position_z) {
    if (!entities_.contains(entity) || !std::isfinite(position_x) ||
        !std::isfinite(position_y) || !std::isfinite(position_z)) {
        throw std::invalid_argument{"Invalid authoritative entity teleport"};
    }
    auto& state = entities_.at(entity);
    state.position_x = position_x;
    state.position_y = position_y;
    state.position_z = position_z;
    state.velocity_x = 0.0F;
    state.velocity_y = 0.0F;
    state.velocity_z = 0.0F;
    state.grounded = position_y <= 0.0F;
    state.air_dodge_available = false;
    state.simulation_tick = simulation_tick_;
}

void AuthoritativeMovementServer::set_entity_collision_enabled(
    const NetworkEntityId entity,
    const bool enabled) {
    auto found = entity_collision_enabled_.find(entity);
    if (found == entity_collision_enabled_.end()) {
        throw std::invalid_argument{"Cannot change collision for an unknown entity"};
    }
    found->second = enabled;
}

void AuthoritativeMovementServer::receive(const ProtocolMessage& message) {
    receive(controlled_entity_, message);
}

void AuthoritativeMovementServer::receive(const NetworkEntityId controlled_entity,
                                          const ProtocolMessage& message) {
    auto client_iterator = clients_.find(controlled_entity);
    if (client_iterator == clients_.end() || !entities_.contains(controlled_entity)) {
        throw std::invalid_argument{"Input connection has no controlled entity"};
    }
    auto& client = client_iterator->second;
    const auto inputs = decode_movement_input_batch(message);
    if (!inputs) {
        throw std::invalid_argument{inputs.error()};
    }
    if (message.acknowledged_sequence != 0 &&
        client.snapshot_history.contains(message.acknowledged_sequence) &&
        (!client.has_acknowledged_snapshot ||
         sequence_more_recent(message.acknowledged_sequence, client.acknowledged_snapshot))) {
        client.acknowledged_snapshot = message.acknowledged_sequence;
        client.has_acknowledged_snapshot = true;
    }
    ++client.input_metrics.batches;
    client.input_metrics.commands += inputs->size();
    client.input_metrics.payload_bytes += message.payload.size();
    client.input_metrics.maximum_batch_size =
        std::max(client.input_metrics.maximum_batch_size, inputs->size());

    bool pending_jump = client.pending_input && client.pending_input->jump;
    bool pending_dodge = client.pending_input && client.pending_input->dodge;
    for (std::size_t index = 0; index < inputs->size(); ++index) {
        const auto& input = (*inputs)[index];
        if ((client.has_acknowledged_input &&
             !sequence_more_recent(input.sequence, client.acknowledged_input)) ||
            !client.pending_input_sequences.insert(input.sequence).second) {
            ++client.input_metrics.duplicate_commands;
            continue;
        }
        if (index + 1 < inputs->size()) {
            ++client.input_metrics.redundant_commands;
        }
        pending_jump = pending_jump || input.jump;
        pending_dodge = pending_dodge || input.dodge;
        if (!client.pending_input ||
            sequence_more_recent(input.sequence, client.pending_input->sequence)) {
            client.pending_input = input;
        }
    }
    if (client.pending_input) {
        client.pending_input->jump = pending_jump;
        client.pending_input->dodge = pending_dodge;
    }
}

std::optional<ProtocolMessage> AuthoritativeMovementServer::tick() {
    ++simulation_tick_;
    for (auto& [controlled_entity, client] : clients_) {
        if (!entities_.contains(controlled_entity)) {
            throw std::logic_error{"A controlled entity has not been added to the server"};
        }
        if (client.pending_input) {
            entity_inputs_[controlled_entity] = *client.pending_input;
            client.acknowledged_input = client.pending_input->sequence;
            client.has_acknowledged_input = true;
            client.pending_input.reset();
            client.pending_input_sequences.clear();
        }
    }
    const double delta = 1.0 / static_cast<double>(settings_.tick_rate);
    for (auto& [entity_id, state] : entities_) {
        auto input = entity_inputs_.at(entity_id);
        input.simulation_tick = simulation_tick_;
        state = simulate_movement_unchecked(state, input, delta, settings_);
        entity_inputs_[entity_id].jump = false;
        entity_inputs_[entity_id].dodge = false;
    }
    if (settings_.resolve_entity_collisions) {
        std::vector<NetworkEntityId> collision_entities;
        collision_entities.reserve(entities_.size());
        for (const auto& [entity, enabled] : entity_collision_enabled_) {
            if (enabled) {
                collision_entities.push_back(entity);
            }
        }
        std::ranges::sort(collision_entities);
        const float minimum_distance = settings_.character_radius * 2.0F;
        const float minimum_distance_squared = minimum_distance * minimum_distance;
        for (std::size_t left_index = 0; left_index < collision_entities.size(); ++left_index) {
            for (std::size_t right_index = left_index + 1;
                 right_index < collision_entities.size(); ++right_index) {
                auto& left = entities_.at(collision_entities[left_index]);
                auto& right = entities_.at(collision_entities[right_index]);
                if (std::abs(left.position_y - right.position_y) >=
                    settings_.character_collision_height) {
                    continue;
                }
                float difference_x = right.position_x - left.position_x;
                float difference_z = right.position_z - left.position_z;
                const float distance_squared = difference_x * difference_x +
                                               difference_z * difference_z;
                if (distance_squared >= minimum_distance_squared) {
                    continue;
                }
                const float distance = std::sqrt(distance_squared);
                if (distance > 1.0e-5F) {
                    difference_x /= distance;
                    difference_z /= distance;
                } else {
                    difference_x = 1.0F;
                    difference_z = 0.0F;
                }
                const float overlap = minimum_distance - distance;
                const float left_x = left.position_x - difference_x * overlap * 0.5F;
                const float left_z = left.position_z - difference_z * overlap * 0.5F;
                const float right_x = right.position_x + difference_x * overlap * 0.5F;
                const float right_z = right.position_z + difference_z * overlap * 0.5F;
                const bool left_blocked = horizontal_motion_blocked(
                    left_x, left_z, left.position_y, settings_);
                const bool right_blocked = horizontal_motion_blocked(
                    right_x, right_z, right.position_y, settings_);
                if (!left_blocked) {
                    left.position_x = left_x;
                    left.position_z = left_z;
                }
                if (!right_blocked) {
                    right.position_x = right_x;
                    right.position_z = right_z;
                }
                if (left_blocked && !right_blocked) {
                    const float full_x = right.position_x + difference_x * overlap * 0.5F;
                    const float full_z = right.position_z + difference_z * overlap * 0.5F;
                    if (!horizontal_motion_blocked(full_x, full_z,
                                                   right.position_y, settings_)) {
                        right.position_x = full_x;
                        right.position_z = full_z;
                    }
                } else if (right_blocked && !left_blocked) {
                    const float full_x = left.position_x - difference_x * overlap * 0.5F;
                    const float full_z = left.position_z - difference_z * overlap * 0.5F;
                    if (!horizontal_motion_blocked(full_x, full_z,
                                                   left.position_y, settings_)) {
                        left.position_x = full_x;
                        left.position_z = full_z;
                    }
                }
                const float left_inward = left.velocity_x * difference_x +
                                          left.velocity_z * difference_z;
                if (left_inward > 0.0F) {
                    left.velocity_x -= difference_x * left_inward;
                    left.velocity_z -= difference_z * left_inward;
                }
                const float right_inward = right.velocity_x * difference_x +
                                           right.velocity_z * difference_z;
                if (right_inward < 0.0F) {
                    right.velocity_x -= difference_x * right_inward;
                    right.velocity_z -= difference_z * right_inward;
                }
            }
        }
    }
    if (simulation_tick_ % (settings_.tick_rate / settings_.snapshot_rate) != 0) {
        return std::nullopt;
    }
    for (auto& [controlled_entity, client] : clients_) {
        client.queued_snapshot = create_snapshot(controlled_entity, client);
    }
    return take_snapshot(controlled_entity_);
}

ProtocolMessage AuthoritativeMovementServer::create_snapshot(
    const NetworkEntityId controlled_entity,
    ClientReplicationState& client) {
    WorldSnapshot snapshot{.simulation_tick = simulation_tick_,
                           .acknowledged_input = client.acknowledged_input};
    client.snapshot_metrics.entities_considered += entities_.size();
    snapshot.entities.reserve(std::min(entities_.size(), settings_.maximum_snapshot_entities));
    const auto& origin = entities_.at(controlled_entity);
    const float relevance_squared = settings_.relevance_radius * settings_.relevance_radius;
    for (const auto& [id, state] : entities_) {
        const float difference_x = state.position_x - origin.position_x;
        const float difference_z = state.position_z - origin.position_z;
        if (id == controlled_entity || settings_.relevance_radius == 0.0F ||
            difference_x * difference_x + difference_z * difference_z <= relevance_squared) {
            snapshot.entities.push_back(state);
        }
    }
    std::ranges::sort(snapshot.entities, [&](const MovementState& left,
                                            const MovementState& right) {
        if (left.entity == controlled_entity || right.entity == controlled_entity) {
            return left.entity == controlled_entity && right.entity != controlled_entity;
        }
        const auto distance_squared = [&](const MovementState& state) {
            const float x = state.position_x - origin.position_x;
            const float z = state.position_z - origin.position_z;
            return x * x + z * z;
        };
        const float left_distance = distance_squared(left);
        const float right_distance = distance_squared(right);
        return left_distance == right_distance ? left.entity < right.entity
                                               : left_distance < right_distance;
    });
    if (snapshot.entities.size() > settings_.maximum_snapshot_entities) {
        snapshot.entities.resize(settings_.maximum_snapshot_entities);
    }
    std::ranges::sort(snapshot.entities, {}, &MovementState::entity);

    const std::uint32_t sequence = ++client.snapshot_sequence;
    ProtocolMessage encoded;
    const auto baseline = client.has_acknowledged_snapshot
                              ? client.snapshot_history.find(client.acknowledged_snapshot)
                              : client.snapshot_history.end();
    if (baseline != client.snapshot_history.end()) {
        encoded = encode_world_snapshot_delta(
            snapshot, sequence, baseline->second, client.acknowledged_snapshot);
        ++client.snapshot_metrics.delta_snapshots;
    } else {
        if (client.has_acknowledged_snapshot) {
            ++client.snapshot_metrics.baseline_misses;
        }
        encoded = encode_world_snapshot(snapshot, sequence);
        ++client.snapshot_metrics.full_snapshots;
    }
    std::size_t count_offset = sizeof(std::uint32_t);
    client.snapshot_metrics.records_encoded +=
        read_integer<std::uint16_t>(encoded.payload, count_offset);
    client.snapshot_metrics.payload_bytes += encoded.payload.size();

    client.snapshot_history.insert_or_assign(sequence, snapshot);
    client.snapshot_history_order.push_back(sequence);
    while (client.snapshot_history_order.size() > settings_.snapshot_capacity) {
        client.snapshot_history.erase(client.snapshot_history_order.front());
        client.snapshot_history_order.erase(client.snapshot_history_order.begin());
    }
    return encoded;
}

std::optional<ProtocolMessage>
AuthoritativeMovementServer::take_snapshot(const NetworkEntityId controlled_entity) {
    auto& queued = clients_.at(controlled_entity).queued_snapshot;
    auto result = std::move(queued);
    queued.reset();
    return result;
}

const MovementState& AuthoritativeMovementServer::entity(const NetworkEntityId id) const {
    return entities_.at(id);
}

WorldSnapshot AuthoritativeMovementServer::capture_world_snapshot() const {
    WorldSnapshot snapshot{
        .simulation_tick = simulation_tick_,
        .acknowledged_input = clients_.at(controlled_entity_).acknowledged_input};
    snapshot.entities.reserve(entities_.size());
    for (const auto& [id, state] : entities_) {
        static_cast<void>(id);
        snapshot.entities.push_back(state);
    }
    std::ranges::sort(snapshot.entities, {}, &MovementState::entity);
    return snapshot;
}

std::uint64_t AuthoritativeMovementServer::simulation_tick() const noexcept {
    return simulation_tick_;
}

std::uint32_t AuthoritativeMovementServer::acknowledged_input() const noexcept {
    return clients_.find(controlled_entity_)->second.acknowledged_input;
}

std::uint32_t AuthoritativeMovementServer::acknowledged_input(
    const NetworkEntityId controlled_entity) const {
    return clients_.at(controlled_entity).acknowledged_input;
}

const InputBatchMetrics& AuthoritativeMovementServer::input_metrics() const noexcept {
    return clients_.find(controlled_entity_)->second.input_metrics;
}

const InputBatchMetrics& AuthoritativeMovementServer::input_metrics(
    const NetworkEntityId controlled_entity) const {
    return clients_.at(controlled_entity).input_metrics;
}

const SnapshotMetrics& AuthoritativeMovementServer::snapshot_metrics() const noexcept {
    return clients_.find(controlled_entity_)->second.snapshot_metrics;
}

const SnapshotMetrics& AuthoritativeMovementServer::snapshot_metrics(
    const NetworkEntityId controlled_entity) const {
    return clients_.at(controlled_entity).snapshot_metrics;
}

PredictedMovementClient::PredictedMovementClient(const ReplicationSettings settings,
                                                 const NetworkEntityId local_entity,
                                                 MovementState initial_state)
    : settings_{settings},
      local_entity_{local_entity},
      local_state_{initial_state},
      prediction_{settings.prediction_capacity} {
    validate_settings(settings_);
    if (local_entity_ == 0 || initial_state.entity != local_entity_ || !finite_state(initial_state)) {
        throw std::invalid_argument{"Predicted client initial state is invalid"};
    }
}

void PredictedMovementClient::set_collision_entities(
    const std::span<const MovementState> entities) {
    collision_entities_.assign(entities.begin(), entities.end());
    std::ranges::sort(collision_entities_, {}, &MovementState::entity);
    for (std::size_t index = 0; index < collision_entities_.size(); ++index) {
        const auto& entity = collision_entities_[index];
        if (entity.entity == 0 || entity.entity == local_entity_ || !finite_state(entity) ||
            (index > 0 && collision_entities_[index - 1].entity == entity.entity)) {
            throw std::invalid_argument{"Predicted collision entities are invalid"};
        }
    }
}

void PredictedMovementClient::reset_local_state(MovementState state) {
    if (state.entity != local_entity_ || !finite_state(state)) {
        throw std::invalid_argument{"Predicted reset state is invalid"};
    }
    local_state_ = state;
    prediction_.clear();
}

MovementState PredictedMovementClient::simulate_local(MovementState state,
                                                       const MovementInput& input) const {
    const double delta = 1.0 / static_cast<double>(settings_.tick_rate);
    state = simulate_movement_unchecked(state, input, delta, settings_);
    if (!settings_.resolve_entity_collisions) {
        return state;
    }
    const float minimum_distance = settings_.character_radius * 2.0F;
    const float minimum_distance_squared = minimum_distance * minimum_distance;
    for (const auto& obstacle : collision_entities_) {
        if (std::abs(state.position_y - obstacle.position_y) >=
            settings_.character_collision_height) {
            continue;
        }
        float outward_x = state.position_x - obstacle.position_x;
        float outward_z = state.position_z - obstacle.position_z;
        const float distance_squared = outward_x * outward_x + outward_z * outward_z;
        if (distance_squared >= minimum_distance_squared) {
            continue;
        }
        const float distance = std::sqrt(distance_squared);
        if (distance > 1.0e-5F) {
            outward_x /= distance;
            outward_z /= distance;
        } else {
            outward_x = local_entity_ < obstacle.entity ? -1.0F : 1.0F;
            outward_z = 0.0F;
        }
        const float correction = (minimum_distance - distance) * 0.5F;
        const float corrected_x = state.position_x + outward_x * correction;
        const float corrected_z = state.position_z + outward_z * correction;
        if (!horizontal_motion_blocked(corrected_x, corrected_z, state.position_y, settings_)) {
            state.position_x = corrected_x;
            state.position_z = corrected_z;
        }
        const float inward_velocity =
            state.velocity_x * outward_x + state.velocity_z * outward_z;
        if (inward_velocity < 0.0F) {
            state.velocity_x -= outward_x * inward_velocity;
            state.velocity_z -= outward_z * inward_velocity;
        }
    }
    return state;
}

ProtocolMessage PredictedMovementClient::create_input(const float axis_x,
                                                      const float axis_z,
                                                      const bool jump, const bool dodge) {
    MovementInput input{.sequence = next_input_sequence_++,
                        .simulation_tick = local_state_.simulation_tick + 1,
                        .axis_x = axis_x,
                        .axis_z = axis_z,
                        .jump = jump, .dodge = dodge};
    std::vector<MovementInput> batch;
    const auto& pending = prediction_.pending();
    const std::size_t previous_count =
        std::min(settings_.input_redundancy - 1, pending.size());
    batch.reserve(previous_count + 1);
    const auto first = pending.end() - static_cast<std::ptrdiff_t>(previous_count);
    for (auto iterator = first; iterator != pending.end(); ++iterator) {
        batch.push_back(iterator->input);
    }
    batch.push_back(input);
    auto message = encode_movement_input_batch(batch);
    if (has_received_snapshot_) {
        message.acknowledged_sequence = latest_snapshot_sequence_;
    }
    local_state_ = simulate_local(local_state_, input);
    if (!prediction_.record({input.sequence, input, local_state_})) {
        throw std::logic_error{"Predicted input sequence was not monotonic"};
    }
    ++input_metrics_.batches;
    input_metrics_.commands += batch.size();
    input_metrics_.redundant_commands += batch.size() - 1;
    input_metrics_.payload_bytes += message.payload.size();
    input_metrics_.maximum_batch_size =
        std::max(input_metrics_.maximum_batch_size, batch.size());
    return message;
}

void PredictedMovementClient::receive(const ProtocolMessage& message) {
    if (has_received_snapshot_ &&
        !sequence_more_recent(message.sequence, latest_snapshot_sequence_)) {
        return;
    }
    const auto baseline_sequence = world_snapshot_baseline_sequence(message);
    if (!baseline_sequence) {
        throw std::invalid_argument{baseline_sequence.error()};
    }
    const WorldSnapshot* baseline = nullptr;
    if (*baseline_sequence != 0) {
        const auto found = snapshot_history_.find(*baseline_sequence);
        if (found == snapshot_history_.end()) {
            ++snapshot_metrics_.baseline_misses;
            throw std::invalid_argument{"Delta snapshot refers to an unavailable baseline"};
        }
        baseline = &found->second;
    }
    const auto snapshot = decode_world_snapshot(message, baseline);
    if (!snapshot) {
        throw std::invalid_argument{snapshot.error()};
    }
    latest_snapshot_sequence_ = message.sequence;
    has_received_snapshot_ = true;
    if (message.flags == MessageFlags::delta_snapshot) {
        ++snapshot_metrics_.delta_snapshots;
    } else {
        ++snapshot_metrics_.full_snapshots;
    }
    std::size_t count_offset = sizeof(std::uint32_t);
    snapshot_metrics_.records_encoded +=
        read_integer<std::uint16_t>(message.payload, count_offset);
    snapshot_metrics_.payload_bytes += message.payload.size();
    snapshot_metrics_.entities_considered += snapshot->entities.size();
    snapshot_history_.insert_or_assign(message.sequence, *snapshot);
    snapshot_history_order_.push_back(message.sequence);
    while (snapshot_history_order_.size() > settings_.snapshot_capacity) {
        snapshot_history_.erase(snapshot_history_order_.front());
        snapshot_history_order_.erase(snapshot_history_order_.begin());
    }

    std::unordered_set<NetworkEntityId> visible_entities;
    visible_entities.reserve(snapshot->entities.size());
    for (const auto& state : snapshot->entities) {
        visible_entities.insert(state.entity);
    }
    std::erase_if(remote_snapshots_, [&](const auto& item) {
        return !visible_entities.contains(item.first);
    });
    const double server_time = static_cast<double>(snapshot->simulation_tick) /
                               static_cast<double>(settings_.tick_rate);
    for (const auto& state : snapshot->entities) {
        if (state.entity == local_entity_) {
            const MovementState previous_prediction = local_state_;
            local_state_ = prediction_.reconcile(
                state, snapshot->acknowledged_input, [&](MovementState replay_state,
                                                          const MovementInput& input) {
                    return simulate_local(replay_state, input);
                });
            reconciliation_metrics_.last_delta_x =
                local_state_.position_x - previous_prediction.position_x;
            reconciliation_metrics_.last_delta_y =
                local_state_.position_y - previous_prediction.position_y;
            reconciliation_metrics_.last_delta_z =
                local_state_.position_z - previous_prediction.position_z;
            const float x = reconciliation_metrics_.last_delta_x;
            const float y = reconciliation_metrics_.last_delta_y;
            const float z = reconciliation_metrics_.last_delta_z;
            reconciliation_metrics_.last_distance = std::sqrt(x * x + y * y + z * z);
            reconciliation_metrics_.maximum_distance = std::max(
                reconciliation_metrics_.maximum_distance, reconciliation_metrics_.last_distance);
            reconciliation_metrics_.accumulated_distance += reconciliation_metrics_.last_distance;
            ++reconciliation_metrics_.count;
        } else {
            auto [buffer, inserted] = remote_snapshots_.try_emplace(
                state.entity, settings_.snapshot_capacity);
            static_cast<void>(inserted);
            static_cast<void>(buffer->second.insert(
                {.simulation_tick = snapshot->simulation_tick,
                 .server_time_seconds = server_time,
                 .state = state}));
        }
    }
}

const MovementState& PredictedMovementClient::local_state() const noexcept {
    return local_state_;
}

std::optional<SnapshotSample<MovementState>>
PredictedMovementClient::sample_remote(const NetworkEntityId entity,
                                       const double estimated_server_time_seconds) const {
    const auto found = remote_snapshots_.find(entity);
    if (found == remote_snapshots_.end()) {
        return std::nullopt;
    }
    const double render_time = estimated_server_time_seconds -
                               settings_.interpolation_delay_seconds;
    return found->second.sample(render_time, interpolate_state);
}

std::uint32_t PredictedMovementClient::pending_input_count() const noexcept {
    return static_cast<std::uint32_t>(prediction_.pending().size());
}

std::uint64_t PredictedMovementClient::reconciliation_count() const noexcept {
    return reconciliation_metrics_.count;
}

float PredictedMovementClient::last_correction_distance() const noexcept {
    return reconciliation_metrics_.last_distance;
}

const ReconciliationMetrics& PredictedMovementClient::reconciliation_metrics() const noexcept {
    return reconciliation_metrics_;
}

const InputBatchMetrics& PredictedMovementClient::input_metrics() const noexcept {
    return input_metrics_;
}

const SnapshotMetrics& PredictedMovementClient::snapshot_metrics() const noexcept {
    return snapshot_metrics_;
}

} // namespace gloom::network
