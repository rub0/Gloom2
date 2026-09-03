#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gloom::network {

enum class SnapshotSampleMode {
    exact,
    interpolated,
    held,
};

template <typename State>
struct TimedSnapshot {
    std::uint64_t simulation_tick{0};
    double server_time_seconds{0.0};
    State state;
};

template <typename State>
struct SnapshotSample {
    State state;
    SnapshotSampleMode mode{SnapshotSampleMode::exact};
    std::uint64_t from_tick{0};
    std::uint64_t to_tick{0};
    double alpha{0.0};
};

// Sorted jitter buffer for remote authoritative state. It deliberately holds the
// newest state during an underrun instead of performing unbounded extrapolation.
template <typename State>
class SnapshotBuffer final {
public:
    explicit SnapshotBuffer(const std::size_t capacity) : capacity_{capacity} {
        if (capacity < 2) {
            throw std::invalid_argument{"Snapshot buffer capacity must be at least two"};
        }
    }

    [[nodiscard]] bool insert(TimedSnapshot<State> snapshot) {
        if (!std::isfinite(snapshot.server_time_seconds)) {
            throw std::invalid_argument{"Snapshot server time must be finite"};
        }
        if (std::ranges::any_of(snapshots_, [&](const auto& existing) {
                return existing.simulation_tick == snapshot.simulation_tick ||
                       existing.server_time_seconds == snapshot.server_time_seconds;
            })) {
            return false;
        }
        const auto position = std::ranges::lower_bound(
            snapshots_, snapshot.server_time_seconds, {}, &TimedSnapshot<State>::server_time_seconds);
        snapshots_.insert(position, std::move(snapshot));
        if (snapshots_.size() > capacity_) {
            snapshots_.erase(snapshots_.begin());
        }
        return true;
    }

    template <typename Interpolator>
    [[nodiscard]] std::optional<SnapshotSample<State>>
    sample(const double target_server_time_seconds, Interpolator&& interpolate) const {
        if (!std::isfinite(target_server_time_seconds)) {
            throw std::invalid_argument{"Snapshot sample time must be finite"};
        }
        if (snapshots_.empty() || target_server_time_seconds < snapshots_.front().server_time_seconds) {
            return std::nullopt;
        }
        const auto right = std::ranges::lower_bound(
            snapshots_, target_server_time_seconds, {}, &TimedSnapshot<State>::server_time_seconds);
        if (right == snapshots_.end()) {
            const auto& latest = snapshots_.back();
            return SnapshotSample<State>{latest.state,
                                         SnapshotSampleMode::held,
                                         latest.simulation_tick,
                                         latest.simulation_tick,
                                         0.0};
        }
        if (right->server_time_seconds == target_server_time_seconds) {
            return SnapshotSample<State>{right->state,
                                         SnapshotSampleMode::exact,
                                         right->simulation_tick,
                                         right->simulation_tick,
                                         0.0};
        }
        const auto left = std::prev(right);
        const double alpha = (target_server_time_seconds - left->server_time_seconds) /
                             (right->server_time_seconds - left->server_time_seconds);
        return SnapshotSample<State>{std::forward<Interpolator>(interpolate)(left->state,
                                                                             right->state,
                                                                             alpha),
                                     SnapshotSampleMode::interpolated,
                                     left->simulation_tick,
                                     right->simulation_tick,
                                     alpha};
    }

    [[nodiscard]] std::size_t size() const noexcept { return snapshots_.size(); }
    [[nodiscard]] bool empty() const noexcept { return snapshots_.empty(); }
    void clear() noexcept { snapshots_.clear(); }

private:
    std::size_t capacity_;
    std::vector<TimedSnapshot<State>> snapshots_;
};

} // namespace gloom::network
