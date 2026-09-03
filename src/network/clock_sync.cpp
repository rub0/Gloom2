#include <gloom/network/clock_sync.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace gloom::network {

void ClockSynchronizer::observe(const ClockExchange& exchange) {
    if (!std::isfinite(exchange.client_send_seconds) ||
        !std::isfinite(exchange.server_receive_seconds) ||
        !std::isfinite(exchange.server_send_seconds) ||
        !std::isfinite(exchange.client_receive_seconds) ||
        exchange.client_receive_seconds < exchange.client_send_seconds ||
        exchange.server_send_seconds < exchange.server_receive_seconds) {
        throw std::invalid_argument{"Clock exchange timestamps are invalid"};
    }
    const double server_processing = exchange.server_send_seconds -
                                     exchange.server_receive_seconds;
    const double round_trip = std::max(
        0.0,
        exchange.client_receive_seconds - exchange.client_send_seconds - server_processing);
    const double offset = ((exchange.server_receive_seconds - exchange.client_send_seconds) +
                           (exchange.server_send_seconds - exchange.client_receive_seconds)) *
                          0.5;
    if (estimate_.samples == 0) {
        estimate_ = {.offset_seconds = offset, .round_trip_seconds = round_trip, .samples = 1};
        best_round_trip_seconds_ = round_trip;
        return;
    }
    best_round_trip_seconds_ = std::min(best_round_trip_seconds_, round_trip);
    const double excess_delay = std::max(0.0, round_trip - best_round_trip_seconds_);
    const double weight = std::clamp(0.2 / (1.0 + excess_delay * 20.0), 0.02, 0.2);
    estimate_.offset_seconds += (offset - estimate_.offset_seconds) * weight;
    estimate_.round_trip_seconds += (round_trip - estimate_.round_trip_seconds) * weight;
    ++estimate_.samples;
}

double ClockSynchronizer::server_time(const double client_time_seconds) const {
    if (!std::isfinite(client_time_seconds)) {
        throw std::invalid_argument{"Client clock time must be finite"};
    }
    return client_time_seconds + estimate_.offset_seconds;
}

ClockEstimate ClockSynchronizer::estimate() const noexcept {
    return estimate_;
}

void ClockSynchronizer::reset() noexcept {
    estimate_ = {};
    best_round_trip_seconds_ = 0.0;
}

} // namespace gloom::network
