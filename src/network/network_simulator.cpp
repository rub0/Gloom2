#include <gloom/network/network_simulator.hpp>

#include <algorithm>
#include <cmath>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>

namespace gloom::network {
namespace {

void validate_probability(const double value, const char* name) {
    if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
        throw std::invalid_argument{std::string{name} + " must be between zero and one"};
    }
}

void validate_duration(const double value, const char* name) {
    if (!std::isfinite(value) || value < 0.0) {
        throw std::invalid_argument{std::string{name} + " must be finite and non-negative"};
    }
}

} // namespace

struct NetworkSimulator::Impl {
    struct ScheduledPacket {
        SimulatedPacket packet;
        double ready_at_seconds{0.0};
        std::uint64_t insertion_order{0};
    };

    struct Later {
        [[nodiscard]] bool operator()(const ScheduledPacket& left,
                                      const ScheduledPacket& right) const noexcept {
            if (left.ready_at_seconds != right.ready_at_seconds) {
                return left.ready_at_seconds > right.ready_at_seconds;
            }
            return left.insertion_order > right.insertion_order;
        }
    };

    explicit Impl(NetworkSimulationSettings configured_settings)
        : settings{configured_settings}, random_state{settings.random_seed} {
        validate_duration(settings.latency_seconds, "Network latency");
        validate_duration(settings.jitter_seconds, "Network jitter");
        validate_duration(settings.maximum_reorder_delay_seconds,
                          "Maximum reorder delay");
        validate_probability(settings.loss_probability, "Packet loss probability");
        validate_probability(settings.duplicate_probability, "Packet duplicate probability");
        validate_probability(settings.reorder_probability, "Packet reorder probability");
        if (random_state == 0) {
            random_state = 0x9e3779b97f4a7c15ULL;
        }
    }

    [[nodiscard]] std::uint64_t next_random() noexcept {
        // xorshift64*: stable across standard-library implementations.
        random_state ^= random_state >> 12;
        random_state ^= random_state << 25;
        random_state ^= random_state >> 27;
        return random_state * 0x2545f4914f6cdd1dULL;
    }

    [[nodiscard]] double random_unit() noexcept {
        constexpr double inverse_53_bits = 1.0 / 9'007'199'254'740'992.0;
        return static_cast<double>(next_random() >> 11) * inverse_53_bits;
    }

    [[nodiscard]] bool chance(const double probability) noexcept {
        return probability > 0.0 && random_unit() < probability;
    }

    [[nodiscard]] double transmission_completion(const std::size_t byte_count) noexcept {
        if (settings.bandwidth_bytes_per_second == 0) {
            return now_seconds;
        }
        const double transmission_start = std::max(now_seconds, next_transmission_seconds);
        const double duration = static_cast<double>(byte_count) /
                                static_cast<double>(settings.bandwidth_bytes_per_second);
        next_transmission_seconds = transmission_start + duration;
        return next_transmission_seconds;
    }

    void schedule_copy(const SimulatedPacketView view, const double transmission_done) {
        double delay = settings.latency_seconds;
        if (settings.jitter_seconds > 0.0) {
            delay += (random_unit() * 2.0 - 1.0) * settings.jitter_seconds;
        }
        delay = std::max(0.0, delay);
        if (chance(settings.reorder_probability)) {
            delay += random_unit() * settings.maximum_reorder_delay_seconds;
        }

        SimulatedPacket packet;
        packet.flow = view.flow;
        packet.sequence = view.sequence;
        packet.payload.assign(view.payload.begin(), view.payload.end());
        packet.queued_at_seconds = now_seconds;
        queue.push({std::move(packet), transmission_done + delay, insertion_order++});
    }

    NetworkSimulationSettings settings;
    std::uint64_t random_state;
    double now_seconds{0.0};
    double next_transmission_seconds{0.0};
    std::uint64_t insertion_order{0};
    std::priority_queue<ScheduledPacket, std::vector<ScheduledPacket>, Later> queue;
    NetworkSimulationMetrics counters;
};

NetworkSimulator::NetworkSimulator(NetworkSimulationSettings settings)
    : impl_{std::make_unique<Impl>(settings)} {}

NetworkSimulator::~NetworkSimulator() = default;
NetworkSimulator::NetworkSimulator(NetworkSimulator&&) noexcept = default;
NetworkSimulator& NetworkSimulator::operator=(NetworkSimulator&&) noexcept = default;

void NetworkSimulator::submit(const SimulatedPacketView packet) {
    ++impl_->counters.submitted_packets;
    impl_->counters.submitted_bytes += packet.payload.size();
    if (impl_->settings.queue_capacity_packets != 0 &&
        impl_->queue.size() >= impl_->settings.queue_capacity_packets) {
        ++impl_->counters.dropped_by_queue;
        return;
    }
    if (impl_->chance(impl_->settings.loss_probability)) {
        ++impl_->counters.dropped_by_loss;
        return;
    }

    const double transmission_done = impl_->transmission_completion(packet.payload.size());
    impl_->schedule_copy(packet, transmission_done);
    if (impl_->chance(impl_->settings.duplicate_probability)) {
        if (impl_->settings.queue_capacity_packets == 0 ||
            impl_->queue.size() < impl_->settings.queue_capacity_packets) {
            impl_->schedule_copy(packet, transmission_done);
            ++impl_->counters.duplicated_packets;
        } else {
            ++impl_->counters.dropped_by_queue;
        }
    }
}

void NetworkSimulator::advance(const double delta_seconds) {
    validate_duration(delta_seconds, "Network simulation delta");
    impl_->now_seconds += delta_seconds;
}

std::optional<SimulatedPacket> NetworkSimulator::receive() {
    if (impl_->queue.empty() || impl_->queue.top().ready_at_seconds > impl_->now_seconds) {
        return std::nullopt;
    }
    auto scheduled = impl_->queue.top();
    impl_->queue.pop();
    scheduled.packet.delivered_at_seconds = scheduled.ready_at_seconds;
    ++impl_->counters.delivered_packets;
    impl_->counters.delivered_bytes += scheduled.packet.payload.size();
    return std::move(scheduled.packet);
}

double NetworkSimulator::time_seconds() const noexcept {
    return impl_->now_seconds;
}

NetworkSimulationMetrics NetworkSimulator::metrics() const noexcept {
    auto result = impl_->counters;
    result.queued_packets = impl_->queue.size();
    return result;
}

void NetworkSimulator::clear() noexcept {
    impl_->queue = {};
    impl_->next_transmission_seconds = impl_->now_seconds;
}

} // namespace gloom::network
