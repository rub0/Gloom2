#pragma once

#include <gloom/network/movement_replication.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace gloom::network {

struct PresentationSmoothingSettings {
    double half_life_seconds{0.10};
    float snap_distance{1.5F};
};

struct PresentationSmoothingMetrics {
    std::uint64_t smoothed_corrections{0};
    std::uint64_t snapped_corrections{0};
    float current_offset_distance{0.0F};
};

// Keeps presentation continuous when prediction reconciliation changes the
// canonical local state. Simulation itself always uses the corrected state.
class PresentationSmoother final {
public:
    explicit PresentationSmoother(PresentationSmoothingSettings settings = {})
        : settings_{settings} {
        if (!std::isfinite(settings_.half_life_seconds) ||
            settings_.half_life_seconds <= 0.0 || !std::isfinite(settings_.snap_distance) ||
            settings_.snap_distance <= 0.0F) {
            throw std::invalid_argument{"Presentation smoothing settings are invalid"};
        }
    }

    void observe_correction(const ReconciliationMetrics& reconciliation) noexcept {
        if (reconciliation.last_distance >= settings_.snap_distance) {
            offset_x_ = 0.0F;
            offset_y_ = 0.0F;
            offset_z_ = 0.0F;
            ++metrics_.snapped_corrections;
        } else if (reconciliation.last_distance > 0.0F) {
            offset_x_ -= reconciliation.last_delta_x;
            offset_y_ -= reconciliation.last_delta_y;
            offset_z_ -= reconciliation.last_delta_z;
            ++metrics_.smoothed_corrections;
        }
        update_offset_distance();
    }

    void advance(const double delta_seconds) {
        if (!std::isfinite(delta_seconds) || delta_seconds < 0.0) {
            throw std::invalid_argument{"Presentation smoothing delta must be finite and non-negative"};
        }
        const float decay = static_cast<float>(
            std::exp2(-delta_seconds / settings_.half_life_seconds));
        offset_x_ *= decay;
        offset_y_ *= decay;
        offset_z_ *= decay;
        update_offset_distance();
    }

    [[nodiscard]] MovementState apply(MovementState state) const noexcept {
        state.position_x += offset_x_;
        state.position_y += offset_y_;
        state.position_z += offset_z_;
        return state;
    }

    [[nodiscard]] const PresentationSmoothingMetrics& metrics() const noexcept {
        return metrics_;
    }

private:
    void update_offset_distance() noexcept {
        metrics_.current_offset_distance =
            std::sqrt(offset_x_ * offset_x_ + offset_y_ * offset_y_ + offset_z_ * offset_z_);
    }

    PresentationSmoothingSettings settings_;
    PresentationSmoothingMetrics metrics_;
    float offset_x_{0.0F};
    float offset_y_{0.0F};
    float offset_z_{0.0F};
};

} // namespace gloom::network
