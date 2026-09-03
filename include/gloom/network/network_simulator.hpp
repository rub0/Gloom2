#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace gloom::network {

struct NetworkSimulationSettings {
    double latency_seconds{0.0};
    double jitter_seconds{0.0};
    double loss_probability{0.0};
    double duplicate_probability{0.0};
    double reorder_probability{0.0};
    double maximum_reorder_delay_seconds{0.0};
    std::uint64_t bandwidth_bytes_per_second{0}; // Zero means unlimited.
    std::size_t queue_capacity_packets{0};       // Zero means unlimited.
    std::uint64_t random_seed{1};
};

struct SimulatedPacketView {
    std::uint64_t flow{0};
    std::uint64_t sequence{0};
    std::span<const std::byte> payload;
};

struct SimulatedPacket {
    std::uint64_t flow{0};
    std::uint64_t sequence{0};
    std::vector<std::byte> payload;
    double queued_at_seconds{0.0};
    double delivered_at_seconds{0.0};
};

struct NetworkSimulationMetrics {
    std::uint64_t submitted_packets{0};
    std::uint64_t delivered_packets{0};
    std::uint64_t duplicated_packets{0};
    std::uint64_t dropped_by_loss{0};
    std::uint64_t dropped_by_queue{0};
    std::uint64_t submitted_bytes{0};
    std::uint64_t delivered_bytes{0};
    std::size_t queued_packets{0};
};

// Deterministic, simulated-time packet conditioner for tests and network labs.
// It owns packet data and performs no sleeping or wall-clock access.
class NetworkSimulator final {
public:
    explicit NetworkSimulator(NetworkSimulationSettings settings = {});
    ~NetworkSimulator();

    NetworkSimulator(const NetworkSimulator&) = delete;
    NetworkSimulator& operator=(const NetworkSimulator&) = delete;
    NetworkSimulator(NetworkSimulator&&) noexcept;
    NetworkSimulator& operator=(NetworkSimulator&&) noexcept;

    void submit(SimulatedPacketView packet);
    void advance(double delta_seconds);
    [[nodiscard]] std::optional<SimulatedPacket> receive();
    [[nodiscard]] double time_seconds() const noexcept;
    [[nodiscard]] NetworkSimulationMetrics metrics() const noexcept;
    void clear() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gloom::network
