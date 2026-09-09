#pragma once

#include <gloom/core/types.hpp>
#include <gloom/gameplay/legacy_pickups.hpp>

namespace gloom::gameplay {

inline constexpr float pickup_appearance_duration = 0.45F;

struct PickupVisual {
    float vertical_offset{};
    float yaw{};
    float scale{1.0F};
    float opacity{1.0F};
    float halo_alpha{};
};

class PickupPresentation {
public:
    void update(const PickupView* pickups, uint32 count, float elapsed_seconds);
    [[nodiscard]] PickupVisual visual(uint32 index, PickupKind kind, float presentation_seconds) const;

private:
    PickupPhase previous_phase_[factory_pickup_count]{};
    float appearance_age_[factory_pickup_count]{};
    uint32 count_{};
    bool initialized_{};
};

[[nodiscard]] PickupVisual pickup_visual(uint32 index, PickupKind kind, float presentation_seconds, float appearance_age);

}
