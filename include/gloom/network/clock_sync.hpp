#pragma once

#include <cstdint>

namespace gloom::network {

struct ClockExchange {
    std::uint64_t nonce{0};
    double client_send_seconds{0.0};
    double server_receive_seconds{0.0};
    double server_send_seconds{0.0};
    double client_receive_seconds{0.0};
};

struct ClockEstimate {
    double offset_seconds{0.0}; // server_time - client_time
    double round_trip_seconds{0.0};
    std::uint64_t samples{0};
};

// NTP-style four-timestamp estimator. Samples close to the best observed RTT
// receive more weight, limiting offset bias from asymmetric queueing.
class ClockSynchronizer final {
public:
    void observe(const ClockExchange& exchange);
    [[nodiscard]] double server_time(double client_time_seconds) const;
    [[nodiscard]] ClockEstimate estimate() const noexcept;
    void reset() noexcept;

private:
    ClockEstimate estimate_;
    double best_round_trip_seconds_{0.0};
};

} // namespace gloom::network
