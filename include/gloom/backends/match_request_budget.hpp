#pragma once
#include <algorithm>
#include <chrono>
#include <mutex>

namespace gloom::backends {
// Fixed-memory token bucket. Callers cannot choose the clock or bucket identity.
class MatchRequestBudget final {
public:
    using Clock = std::chrono::steady_clock;
    MatchRequestBudget(unsigned burst, unsigned per_second, Clock::time_point now = Clock::now())
        : capacity_{static_cast<double>(burst)}, rate_{static_cast<double>(per_second)},
          tokens_{capacity_}, last_{now} {}
    [[nodiscard]] bool consume(Clock::time_point now = Clock::now()) {
        std::lock_guard lock{mutex_};
        if (now > last_) {
            tokens_ = std::min(capacity_, tokens_ + std::chrono::duration<double>(now - last_).count() * rate_);
            last_ = now;
        }
        if (tokens_ < 1.0) return false;
        tokens_ -= 1.0;
        return true;
    }
private:
    const double capacity_, rate_;
    double tokens_;
    Clock::time_point last_;
    std::mutex mutex_;
};
} // namespace gloom::backends
