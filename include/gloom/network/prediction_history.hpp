#pragma once

#include <gloom/network/protocol.hpp>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <stdexcept>
#include <utility>

namespace gloom::network {

template <typename Input, typename State>
struct PredictedInput {
    std::uint32_t sequence{0};
    Input input;
    State predicted_state;
};

// Stores local inputs until the authoritative server acknowledges them. Reconcile
// starts from server state and deterministically replays only unacknowledged inputs.
template <typename Input, typename State>
class PredictionHistory final {
public:
    explicit PredictionHistory(const std::size_t capacity) : capacity_{capacity} {
        if (capacity == 0) {
            throw std::invalid_argument{"Prediction history capacity must be positive"};
        }
    }

    [[nodiscard]] bool record(PredictedInput<Input, State> input) {
        if (!inputs_.empty() &&
            !sequence_more_recent(input.sequence, inputs_.back().sequence)) {
            return false;
        }
        if (inputs_.size() == capacity_) {
            inputs_.pop_front();
            overflowed_ = true;
        }
        inputs_.push_back(std::move(input));
        return true;
    }

    template <typename Simulator>
    [[nodiscard]] State reconcile(State authoritative_state,
                                  const std::uint32_t acknowledged_sequence,
                                  Simulator&& simulate) {
        while (!inputs_.empty() &&
               !sequence_more_recent(inputs_.front().sequence, acknowledged_sequence)) {
            inputs_.pop_front();
        }
        auto&& simulator = simulate;
        for (auto& pending : inputs_) {
            authoritative_state = simulator(authoritative_state, pending.input);
            pending.predicted_state = authoritative_state;
        }
        return authoritative_state;
    }

    [[nodiscard]] const std::deque<PredictedInput<Input, State>>& pending() const noexcept {
        return inputs_;
    }
    [[nodiscard]] bool overflowed() const noexcept { return overflowed_; }
    void clear() noexcept {
        inputs_.clear();
        overflowed_ = false;
    }

private:
    std::size_t capacity_;
    std::deque<PredictedInput<Input, State>> inputs_;
    bool overflowed_{false};
};

} // namespace gloom::network
