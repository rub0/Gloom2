#include <gloom/network/combat.hpp>
#include <gloom/physics/triangle_query.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>

namespace gloom::network {
namespace {

constexpr std::uint8_t fire_command_subtype = 1;
constexpr std::uint8_t fire_result_subtype = 2;
constexpr std::size_t fire_command_payload_size = 25;
constexpr std::size_t fire_result_payload_size = 38;

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

[[nodiscard]] bool valid_direction(const float x, const float y, const float z) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
        return false;
    }
    const float length_squared = x * x + y * y + z * z;
    return length_squared >= 0.99F * 0.99F && length_squared <= 1.01F * 1.01F;
}

[[nodiscard]] bool valid_status(const FireValidationStatus status) noexcept {
    return status >= FireValidationStatus::hit && status <= FireValidationStatus::rejected_invalid;
}

[[nodiscard]] const MovementState* find_entity(const WorldSnapshot& snapshot,
                                               const NetworkEntityId entity) noexcept {
    const auto found = std::ranges::find(snapshot.entities, entity, &MovementState::entity);
    return found == snapshot.entities.end() ? nullptr : &*found;
}

[[nodiscard]] std::optional<float> ray_obstacle_distance(
    const float origin_x,
    const float origin_y,
    const float origin_z,
    const FireCommand& command,
    const StaticMovementObstacle& obstacle) noexcept {
    float minimum_distance = 0.0F;
    float maximum_distance = command.maximum_distance;
    const auto intersect_axis = [&](const float origin,
                                    const float direction,
                                    const float minimum,
                                    const float maximum) {
        if (std::abs(direction) < 1.0e-6F) {
            return origin >= minimum && origin <= maximum;
        }
        float first = (minimum - origin) / direction;
        float second = (maximum - origin) / direction;
        if (first > second) {
            std::swap(first, second);
        }
        minimum_distance = std::max(minimum_distance, first);
        maximum_distance = std::min(maximum_distance, second);
        return minimum_distance <= maximum_distance;
    };
    if (!intersect_axis(origin_x, command.aim_x, obstacle.minimum_x, obstacle.maximum_x) ||
        !intersect_axis(origin_y, command.aim_y, 0.0F, obstacle.top_y) ||
        !intersect_axis(origin_z, command.aim_z, obstacle.minimum_z, obstacle.maximum_z)) {
        return std::nullopt;
    }
    return minimum_distance;
}

[[nodiscard]] std::optional<float> ray_sphere_distance(
    const float origin_x,
    const float origin_y,
    const float origin_z,
    const FireCommand& command,
    const float center_x,
    const float center_y,
    const float center_z,
    const float radius) noexcept {
    const float offset_x = origin_x - center_x;
    const float offset_y = origin_y - center_y;
    const float offset_z = origin_z - center_z;
    const float projection =
        offset_x * command.aim_x + offset_y * command.aim_y + offset_z * command.aim_z;
    const float offset_squared =
        offset_x * offset_x + offset_y * offset_y + offset_z * offset_z;
    const float discriminant = projection * projection - (offset_squared - radius * radius);
    if (discriminant < 0.0F) {
        return std::nullopt;
    }
    const float root = std::sqrt(discriminant);
    const float exit_distance = -projection + root;
    if (exit_distance < 0.0F) {
        return std::nullopt;
    }
    return std::max(0.0F, -projection - root);
}

[[nodiscard]] std::optional<float> ray_vertical_capsule_distance(
    const float origin_x,
    const float origin_y,
    const float origin_z,
    const FireCommand& command,
    const float center_x,
    const float center_y,
    const float center_z,
    const float radius,
    const float half_height) noexcept {
    if (half_height <= 0.0F) {
        return ray_sphere_distance(origin_x, origin_y, origin_z, command, center_x, center_y,
                                   center_z, radius);
    }

    const float minimum_y = center_y - half_height;
    const float maximum_y = center_y + half_height;
    const float offset_x = origin_x - center_x;
    const float offset_z = origin_z - center_z;
    const float radial_squared = offset_x * offset_x + offset_z * offset_z;
    const float clamped_y = std::clamp(origin_y, minimum_y, maximum_y);
    const float vertical_offset = origin_y - clamped_y;
    if (radial_squared + vertical_offset * vertical_offset <= radius * radius) {
        return 0.0F;
    }

    std::optional<float> closest;
    const auto consider = [&closest](const std::optional<float> distance) {
        if (distance && (!closest || *distance < *closest)) {
            closest = distance;
        }
    };
    consider(ray_sphere_distance(origin_x, origin_y, origin_z, command, center_x, minimum_y,
                                 center_z, radius));
    consider(ray_sphere_distance(origin_x, origin_y, origin_z, command, center_x, maximum_y,
                                 center_z, radius));

    const float cylinder_a = command.aim_x * command.aim_x + command.aim_z * command.aim_z;
    if (cylinder_a > 1.0e-8F) {
        const float cylinder_b =
            2.0F * (offset_x * command.aim_x + offset_z * command.aim_z);
        const float cylinder_c = radial_squared - radius * radius;
        const float discriminant = cylinder_b * cylinder_b - 4.0F * cylinder_a * cylinder_c;
        if (discriminant >= 0.0F) {
            const float root = std::sqrt(discriminant);
            const float inverse_denominator = 0.5F / cylinder_a;
            const float distances[2]{(-cylinder_b - root) * inverse_denominator,
                                     (-cylinder_b + root) * inverse_denominator};
            for (const float distance : distances) {
                const float hit_y = origin_y + distance * command.aim_y;
                if (distance >= 0.0F && hit_y >= minimum_y && hit_y <= maximum_y) {
                    consider(distance);
                }
            }
        }
    }
    return closest;
}

} // namespace

ProtocolMessage encode_fire_command(const FireCommand& command) {
    if (command.shooter == 0 || !valid_direction(command.aim_x, command.aim_y, command.aim_z) ||
        !std::isfinite(command.maximum_distance) || command.maximum_distance <= 0.0F) {
        throw std::invalid_argument{"Fire command is invalid"};
    }
    ProtocolMessage message;
    message.kind = MessageKind::event;
    message.sequence = command.sequence;
    message.simulation_tick = command.estimated_server_tick;
    message.payload.reserve(fire_command_payload_size);
    append_integer(message.payload, fire_command_subtype);
    append_integer(message.payload, command.shooter);
    append_float(message.payload, command.aim_x);
    append_float(message.payload, command.aim_y);
    append_float(message.payload, command.aim_z);
    append_float(message.payload, command.maximum_distance);
    return message;
}

std::expected<FireCommand, std::string>
decode_fire_command(const ProtocolMessage& message) {
    if (message.kind != MessageKind::event || message.payload.size() != fire_command_payload_size) {
        return std::unexpected{"Message is not a fire command"};
    }
    std::size_t offset = 0;
    if (read_integer<std::uint8_t>(message.payload, offset) != fire_command_subtype) {
        return std::unexpected{"Event subtype is not a fire command"};
    }
    FireCommand command{
        .sequence = message.sequence,
        .shooter = read_integer<std::uint64_t>(message.payload, offset),
        .estimated_server_tick = message.simulation_tick,
        .aim_x = read_float(message.payload, offset),
        .aim_y = read_float(message.payload, offset),
        .aim_z = read_float(message.payload, offset),
        .maximum_distance = read_float(message.payload, offset),
    };
    if (command.shooter == 0 || !valid_direction(command.aim_x, command.aim_y, command.aim_z) ||
        !std::isfinite(command.maximum_distance) || command.maximum_distance <= 0.0F) {
        return std::unexpected{"Fire command fields are invalid"};
    }
    return command;
}

ProtocolMessage encode_fire_result(const FireResult& result) {
    if (result.shooter == 0 || !valid_status(result.status) || !std::isfinite(result.distance) ||
        result.distance < 0.0F ||
        (result.status == FireValidationStatus::hit && result.target == 0) ||
        (result.status != FireValidationStatus::hit && result.target != 0)) {
        throw std::invalid_argument{"Fire result is invalid"};
    }
    ProtocolMessage message;
    message.kind = MessageKind::event;
    message.sequence = result.fire_sequence;
    message.acknowledged_sequence = result.fire_sequence;
    message.simulation_tick = result.server_tick;
    message.payload.reserve(fire_result_payload_size);
    append_integer(message.payload, fire_result_subtype);
    append_integer(message.payload, static_cast<std::uint8_t>(result.status));
    append_integer(message.payload, result.shooter);
    append_integer(message.payload, result.target);
    append_integer(message.payload, result.requested_tick);
    append_integer(message.payload, result.evaluated_tick);
    append_float(message.payload, result.distance);
    return message;
}

std::expected<FireResult, std::string>
decode_fire_result(const ProtocolMessage& message) {
    if (message.kind != MessageKind::event || message.payload.size() != fire_result_payload_size) {
        return std::unexpected{"Message is not a fire result"};
    }
    std::size_t offset = 0;
    if (read_integer<std::uint8_t>(message.payload, offset) != fire_result_subtype) {
        return std::unexpected{"Event subtype is not a fire result"};
    }
    FireResult result{.fire_sequence = message.acknowledged_sequence};
    result.status = static_cast<FireValidationStatus>(
        read_integer<std::uint8_t>(message.payload, offset));
    result.shooter = read_integer<std::uint64_t>(message.payload, offset);
    result.target = read_integer<std::uint64_t>(message.payload, offset);
    result.requested_tick = read_integer<std::uint64_t>(message.payload, offset);
    result.evaluated_tick = read_integer<std::uint64_t>(message.payload, offset);
    result.server_tick = message.simulation_tick;
    result.distance = read_float(message.payload, offset);
    if (message.sequence != result.fire_sequence || result.shooter == 0 ||
        !valid_status(result.status) || !std::isfinite(result.distance) || result.distance < 0.0F ||
        (result.status == FireValidationStatus::hit && result.target == 0) ||
        (result.status != FireValidationStatus::hit && result.target != 0)) {
        return std::unexpected{"Fire result fields are invalid"};
    }
    return result;
}

LagCompensatedCombatServer::LagCompensatedCombatServer(CombatSettings settings)
    : settings_{settings} {
    if (settings_.history_ticks == 0 || !std::isfinite(settings_.hit_radius) ||
        settings_.hit_radius <= 0.0F || !std::isfinite(settings_.character_center_height) ||
        settings_.character_center_height < 0.0F || !std::isfinite(settings_.hit_half_height) ||
        settings_.hit_half_height < 0.0F || !std::isfinite(settings_.maximum_range) ||
        settings_.maximum_range <= 0.0F ||
        !std::ranges::all_of(settings_.static_obstacles, [](const auto& obstacle) {
            return std::isfinite(obstacle.minimum_x) && std::isfinite(obstacle.maximum_x) &&
                   std::isfinite(obstacle.minimum_z) && std::isfinite(obstacle.maximum_z) &&
                   std::isfinite(obstacle.top_y) && obstacle.minimum_x < obstacle.maximum_x &&
                   obstacle.minimum_z < obstacle.maximum_z && obstacle.top_y > 0.0F;
        })) {
        throw std::invalid_argument{"Combat settings are invalid"};
    }
    if (settings_.static_mesh) {
        const auto& mesh=*settings_.static_mesh;
        if (mesh.indices.empty() || mesh.indices.size()%3 ||
            !std::ranges::all_of(mesh.indices,[&](auto i){return i<mesh.vertices.size();}) ||
            !std::ranges::all_of(mesh.vertices,[](auto p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}))
            throw std::invalid_argument{"Combat triangle mesh is invalid"};
    }
}

void LagCompensatedCombatServer::record(WorldSnapshot snapshot) {
    if (!history_.empty() && snapshot.simulation_tick <= history_.back().simulation_tick) {
        throw std::invalid_argument{"Combat history ticks must be strictly increasing"};
    }
    std::ranges::sort(snapshot.entities, {}, &MovementState::entity);
    for (std::size_t index = 0; index < snapshot.entities.size(); ++index) {
        const auto& entity = snapshot.entities[index];
        if (entity.entity == 0 || !std::isfinite(entity.position_x) ||
            !std::isfinite(entity.position_y) || !std::isfinite(entity.position_z) ||
            (index > 0 && snapshot.entities[index - 1].entity == entity.entity)) {
            throw std::invalid_argument{"Combat history contains invalid entities"};
        }
    }
    history_.push_back(std::move(snapshot));
    while (history_.size() > settings_.history_ticks) {
        history_.pop_front();
    }
    ++metrics_.history_frames;
}

FireResult LagCompensatedCombatServer::validate(const FireCommand& command) {
    const std::uint64_t server_tick = history_.empty() ? 0 : history_.back().simulation_tick;
    FireResult result{
        .fire_sequence = command.sequence,
        .shooter = command.shooter,
        .requested_tick = command.estimated_server_tick,
        .server_tick = server_tick,
    };
    if (history_.empty() || command.shooter == 0 ||
        !valid_direction(command.aim_x, command.aim_y, command.aim_z) ||
        !std::isfinite(command.maximum_distance) || command.maximum_distance <= 0.0F ||
        command.maximum_distance > settings_.maximum_range) {
        result.status = FireValidationStatus::rejected_invalid;
        ++metrics_.invalid_rejections;
        return result;
    }
    const auto latest = latest_fire_sequences_.find(command.shooter);
    if (latest != latest_fire_sequences_.end() &&
        !sequence_more_recent(command.sequence, latest->second)) {
        result.status = FireValidationStatus::rejected_duplicate;
        ++metrics_.duplicate_rejections;
        return result;
    }
    if (command.estimated_server_tick > server_tick + settings_.future_tolerance_ticks) {
        result.status = FireValidationStatus::rejected_future;
        ++metrics_.future_rejections;
        return result;
    }
    if (command.estimated_server_tick < history_.front().simulation_tick) {
        result.status = FireValidationStatus::rejected_too_old;
        ++metrics_.too_old_rejections;
        return result;
    }

    const std::uint64_t target_tick = std::min(command.estimated_server_tick, server_tick);
    auto frame = std::ranges::upper_bound(
        history_, target_tick, {}, &WorldSnapshot::simulation_tick);
    if (frame == history_.begin()) {
        frame = history_.begin();
    } else {
        --frame;
    }
    result.evaluated_tick = frame->simulation_tick;
    const MovementState* shooter = find_entity(*frame, command.shooter);
    if (shooter == nullptr) {
        result.status = FireValidationStatus::rejected_invalid;
        ++metrics_.invalid_rejections;
        return result;
    }

    latest_fire_sequences_[command.shooter] = command.sequence;
    ++metrics_.accepted_commands;

    const float origin_x = shooter->position_x;
    const float origin_y = shooter->position_y + settings_.character_center_height;
    const float origin_z = shooter->position_z;
    float closest_distance = command.maximum_distance;
    if (settings_.static_mesh) closest_distance=physics::ray_triangle_distance(
        *settings_.static_mesh,{origin_x,origin_y,origin_z},
        {command.aim_x,command.aim_y,command.aim_z},closest_distance);
    for (const auto& obstacle : settings_.static_obstacles) {
        if (const auto distance = ray_obstacle_distance(
                origin_x, origin_y, origin_z, command, obstacle)) {
            closest_distance = std::min(closest_distance, *distance);
        }
    }
    NetworkEntityId closest_target = 0;
    for (const auto& candidate : frame->entities) {
        if (candidate.entity == command.shooter) {
            continue;
        }
        const auto entry_distance = ray_vertical_capsule_distance(
            origin_x, origin_y, origin_z, command, candidate.position_x,
            candidate.position_y + settings_.character_center_height, candidate.position_z,
            settings_.hit_radius, settings_.hit_half_height);
        if (entry_distance && *entry_distance <= closest_distance) {
            closest_distance = *entry_distance;
            closest_target = candidate.entity;
        }
    }
    if (closest_target != 0) {
        result.status = FireValidationStatus::hit;
        result.target = closest_target;
        result.distance = closest_distance;
        ++metrics_.hits;
    } else {
        result.status = FireValidationStatus::miss;
        result.distance = closest_distance;
        ++metrics_.misses;
    }
    return result;
}

std::size_t LagCompensatedCombatServer::history_size() const noexcept {
    return history_.size();
}

const CombatMetrics& LagCompensatedCombatServer::metrics() const noexcept {
    return metrics_;
}

} // namespace gloom::network
