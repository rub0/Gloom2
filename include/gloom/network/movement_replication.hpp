#pragma once

#include <gloom/network/prediction_history.hpp>
#include <gloom/network/protocol.hpp>
#include <gloom/network/snapshot_buffer.hpp>

#include <cstdint>
#include <expected>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gloom::network {

using NetworkEntityId = std::uint64_t;

struct MovementInput {
    std::uint32_t sequence{0};
    std::uint64_t simulation_tick{0};
    float axis_x{0.0F};
    float axis_z{0.0F};
    bool jump{false};
    bool dodge{false};
};

struct MovementState {
    NetworkEntityId entity{0};
    std::uint64_t simulation_tick{0};
    float position_x{0.0F};
    float position_y{0.0F};
    float position_z{0.0F};
    float velocity_x{0.0F};
    float velocity_y{0.0F};
    float velocity_z{0.0F};
    bool grounded{true};
    bool air_dodge_available{false};
};

struct StaticMovementObstacle {
    float minimum_x{0.0F};
    float maximum_x{0.0F};
    float minimum_z{0.0F};
    float maximum_z{0.0F};
    float top_y{0.0F};
};

struct WorldSnapshot {
    std::uint64_t simulation_tick{0};
    std::uint32_t acknowledged_input{0};
    std::vector<MovementState> entities;
};

inline constexpr std::size_t maximum_movement_input_batch = 32;

[[nodiscard]] ProtocolMessage encode_movement_input(const MovementInput& input);
[[nodiscard]] std::expected<MovementInput, std::string>
decode_movement_input(const ProtocolMessage& message);
[[nodiscard]] ProtocolMessage encode_movement_input_batch(
    std::span<const MovementInput> inputs);
[[nodiscard]] std::expected<std::vector<MovementInput>, std::string>
decode_movement_input_batch(const ProtocolMessage& message);
[[nodiscard]] ProtocolMessage encode_world_snapshot(const WorldSnapshot& snapshot,
                                                    std::uint32_t sequence);
[[nodiscard]] ProtocolMessage encode_world_snapshot_delta(const WorldSnapshot& snapshot,
                                                          std::uint32_t sequence,
                                                          const WorldSnapshot& baseline,
                                                          std::uint32_t baseline_sequence);
[[nodiscard]] std::expected<WorldSnapshot, std::string>
decode_world_snapshot(const ProtocolMessage& message,
                      const WorldSnapshot* baseline = nullptr);
[[nodiscard]] std::expected<std::uint32_t, std::string>
world_snapshot_baseline_sequence(const ProtocolMessage& message);

[[nodiscard]] MovementState simulate_movement(MovementState state,
                                              const MovementInput& input,
                                              double fixed_delta_seconds,
                                              const struct ReplicationSettings& settings);

struct ReplicationSettings {
    std::uint32_t tick_rate{60};
    std::uint32_t snapshot_rate{20};
    float movement_speed{5.0F};
    double interpolation_delay_seconds{0.1};
    std::size_t prediction_capacity{256};
    std::size_t snapshot_capacity{64};
    float jump_speed{5.5F};
    float gravity{-15.0F};
    float character_radius{0.4F};
    float character_collision_height{1.8F};
    bool resolve_entity_collisions{false};
    std::vector<StaticMovementObstacle> static_obstacles;
    std::size_t input_redundancy{4};
    float relevance_radius{0.0F};
    std::size_t maximum_snapshot_entities{1024};
    std::function<MovementState(MovementState, const MovementInput&, double)> scene_movement;
};

struct InputBatchMetrics {
    std::uint64_t batches{0};
    std::uint64_t commands{0};
    std::uint64_t redundant_commands{0};
    std::uint64_t duplicate_commands{0};
    std::uint64_t payload_bytes{0};
    std::size_t maximum_batch_size{0};
};

struct ReconciliationMetrics {
    std::uint64_t count{0};
    float last_distance{0.0F};
    float maximum_distance{0.0F};
    double accumulated_distance{0.0};
    float last_delta_x{0.0F};
    float last_delta_y{0.0F};
    float last_delta_z{0.0F};
};

struct SnapshotMetrics {
    std::uint64_t full_snapshots{0};
    std::uint64_t delta_snapshots{0};
    std::uint64_t baseline_misses{0};
    std::uint64_t entities_considered{0};
    std::uint64_t records_encoded{0};
    std::uint64_t payload_bytes{0};
};

class AuthoritativeMovementServer final {
public:
    AuthoritativeMovementServer(ReplicationSettings settings,
                                NetworkEntityId controlled_entity);

    void add_entity(MovementState state);
    void register_client(NetworkEntityId controlled_entity);
    void unregister_client(NetworkEntityId controlled_entity);
    void set_entity_input(NetworkEntityId entity, float axis_x, float axis_z);
    void teleport_entity(NetworkEntityId entity, float position_x, float position_y,
                         float position_z);
    void set_entity_collision_enabled(NetworkEntityId entity, bool enabled);
    void receive(const ProtocolMessage& message);
    void receive(NetworkEntityId controlled_entity, const ProtocolMessage& message);
    [[nodiscard]] std::optional<ProtocolMessage> tick();
    [[nodiscard]] std::optional<ProtocolMessage>
    take_snapshot(NetworkEntityId controlled_entity);
    [[nodiscard]] const MovementState& entity(NetworkEntityId id) const;
    [[nodiscard]] WorldSnapshot capture_world_snapshot() const;
    [[nodiscard]] std::uint64_t simulation_tick() const noexcept;
    [[nodiscard]] std::uint32_t acknowledged_input() const noexcept;
    [[nodiscard]] std::uint32_t
    acknowledged_input(NetworkEntityId controlled_entity) const;
    [[nodiscard]] const InputBatchMetrics& input_metrics() const noexcept;
    [[nodiscard]] const InputBatchMetrics&
    input_metrics(NetworkEntityId controlled_entity) const;
    [[nodiscard]] const SnapshotMetrics& snapshot_metrics() const noexcept;
    [[nodiscard]] const SnapshotMetrics&
    snapshot_metrics(NetworkEntityId controlled_entity) const;

private:
    struct ClientReplicationState {
        std::optional<MovementInput> pending_input;
        std::unordered_set<std::uint32_t> pending_input_sequences;
        std::uint32_t acknowledged_input{0};
        bool has_acknowledged_input{false};
        std::uint32_t snapshot_sequence{0};
        InputBatchMetrics input_metrics;
        std::uint32_t acknowledged_snapshot{0};
        bool has_acknowledged_snapshot{false};
        std::unordered_map<std::uint32_t, WorldSnapshot> snapshot_history;
        std::vector<std::uint32_t> snapshot_history_order;
        SnapshotMetrics snapshot_metrics;
        std::optional<ProtocolMessage> queued_snapshot;
    };

    [[nodiscard]] ProtocolMessage
    create_snapshot(NetworkEntityId controlled_entity, ClientReplicationState& client);

    ReplicationSettings settings_;
    NetworkEntityId controlled_entity_;
    std::unordered_map<NetworkEntityId, MovementState> entities_;
    std::unordered_map<NetworkEntityId, MovementInput> entity_inputs_;
    std::unordered_map<NetworkEntityId, bool> entity_collision_enabled_;
    std::unordered_map<NetworkEntityId, ClientReplicationState> clients_;
    std::uint64_t simulation_tick_{0};
};

class PredictedMovementClient final {
public:
    PredictedMovementClient(ReplicationSettings settings,
                            NetworkEntityId local_entity,
                            MovementState initial_state);

    [[nodiscard]] ProtocolMessage create_input(float axis_x, float axis_z, bool jump = false, bool dodge = false);
    void receive(const ProtocolMessage& message);
    void set_collision_entities(std::span<const MovementState> entities);
    void reset_local_state(MovementState state);
    [[nodiscard]] const MovementState& local_state() const noexcept;
    [[nodiscard]] std::optional<SnapshotSample<MovementState>>
    sample_remote(NetworkEntityId entity, double estimated_server_time_seconds) const;
    [[nodiscard]] std::uint32_t pending_input_count() const noexcept;
    [[nodiscard]] std::uint64_t reconciliation_count() const noexcept;
    [[nodiscard]] float last_correction_distance() const noexcept;
    [[nodiscard]] const ReconciliationMetrics& reconciliation_metrics() const noexcept;
    [[nodiscard]] const InputBatchMetrics& input_metrics() const noexcept;
    [[nodiscard]] const SnapshotMetrics& snapshot_metrics() const noexcept;

private:
    [[nodiscard]] MovementState simulate_local(MovementState state,
                                               const MovementInput& input) const;

    ReplicationSettings settings_;
    NetworkEntityId local_entity_;
    MovementState local_state_;
    PredictionHistory<MovementInput, MovementState> prediction_;
    std::vector<MovementState> collision_entities_;
    std::unordered_map<NetworkEntityId, SnapshotBuffer<MovementState>> remote_snapshots_;
    std::uint32_t next_input_sequence_{1};
    std::uint32_t latest_snapshot_sequence_{0};
    bool has_received_snapshot_{false};
    ReconciliationMetrics reconciliation_metrics_;
    InputBatchMetrics input_metrics_;
    std::unordered_map<std::uint32_t, WorldSnapshot> snapshot_history_;
    std::vector<std::uint32_t> snapshot_history_order_;
    SnapshotMetrics snapshot_metrics_;
};

} // namespace gloom::network
