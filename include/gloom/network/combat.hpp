#pragma once

#include <gloom/network/movement_replication.hpp>
#include <gloom/network/protocol.hpp>
#include <gloom/physics/world.hpp>

#include <cstdint>
#include <deque>
#include <expected>
#include <string>
#include <unordered_map>

namespace gloom::network {

struct FireCommand {
    std::uint32_t sequence{0};
    NetworkEntityId shooter{0};
    std::uint64_t estimated_server_tick{0};
    float aim_x{1.0F};
    float aim_y{0.0F};
    float aim_z{0.0F};
    float maximum_distance{100.0F};
};

enum class FireValidationStatus : std::uint8_t {
    hit = 1,
    miss = 2,
    rejected_duplicate = 3,
    rejected_too_old = 4,
    rejected_future = 5,
    rejected_invalid = 6,
};

struct FireResult {
    std::uint32_t fire_sequence{0};
    NetworkEntityId shooter{0};
    NetworkEntityId target{0};
    std::uint64_t requested_tick{0};
    std::uint64_t evaluated_tick{0};
    std::uint64_t server_tick{0};
    float distance{0.0F};
    FireValidationStatus status{FireValidationStatus::miss};
};

[[nodiscard]] ProtocolMessage encode_fire_command(const FireCommand& command);
[[nodiscard]] std::expected<FireCommand, std::string>
decode_fire_command(const ProtocolMessage& message);
[[nodiscard]] ProtocolMessage encode_fire_result(const FireResult& result);
[[nodiscard]] std::expected<FireResult, std::string>
decode_fire_result(const ProtocolMessage& message);

struct CombatSettings {
    std::size_t history_ticks{60};
    std::uint64_t future_tolerance_ticks{2};
    float hit_radius{0.5F};
    float character_center_height{0.9F};
    float hit_half_height{0.0F};
    float maximum_range{100.0F};
    std::vector<StaticMovementObstacle> static_obstacles;
    std::shared_ptr<const physics::TriangleMesh> static_mesh;
};

struct CombatMetrics {
    std::uint64_t history_frames{0};
    std::uint64_t accepted_commands{0};
    std::uint64_t hits{0};
    std::uint64_t misses{0};
    std::uint64_t duplicate_rejections{0};
    std::uint64_t too_old_rejections{0};
    std::uint64_t future_rejections{0};
    std::uint64_t invalid_rejections{0};
};

class LagCompensatedCombatServer final {
public:
    explicit LagCompensatedCombatServer(CombatSettings settings = {});

    void record(WorldSnapshot snapshot);
    [[nodiscard]] FireResult validate(const FireCommand& command);
    [[nodiscard]] std::size_t history_size() const noexcept;
    [[nodiscard]] const CombatMetrics& metrics() const noexcept;

private:
    CombatSettings settings_;
    std::deque<WorldSnapshot> history_;
    std::unordered_map<NetworkEntityId, std::uint32_t> latest_fire_sequences_;
    CombatMetrics metrics_;
};

} // namespace gloom::network
