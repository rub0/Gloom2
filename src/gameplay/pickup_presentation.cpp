#include <gloom/gameplay/pickup_presentation.hpp>

#include <assert.h>
#include <math.h>

namespace gloom::gameplay {

PickupVisual pickup_visual(const uint32 index, const PickupKind kind, const float presentation_seconds, const float appearance_age) {
    const float phase = static_cast<float>(index) * 2.39996323F;
    const float progress = fminf(fmaxf(appearance_age / pickup_appearance_duration, 0.0F), 1.0F);
    const float eased = progress * progress * (3.0F - 2.0F * progress);
    const float pulse = 0.5F + 0.5F * sinf(presentation_seconds * 2.4F + phase);
    return {
        .vertical_offset = sinf(presentation_seconds * 1.8F + phase) * 0.08F,
        .yaw = presentation_seconds * 0.72F + phase,
        .scale = 0.35F + 0.65F * eased,
        .opacity = eased,
        .halo_alpha = kind == PickupKind::life ? 0.0F : (0.42F + 0.16F * pulse) * eased,
    };
}

void PickupPresentation::update(const PickupView* const pickups, const uint32 count, const float elapsed_seconds) {
    assert(pickups || count == 0);
    assert(count <= factory_pickup_count);
    assert(elapsed_seconds >= 0.0F);
    if (count == 0) return;
    if (!initialized_) {
        count_ = count;
        for (uint32 i = 0; i < count; ++i) {
            previous_phase_[i] = pickups[i].phase;
            appearance_age_[i] = pickups[i].phase == PickupPhase::respawning ? pickup_appearance_duration : 0.0F;
        }
        initialized_ = true;
        return;
    }
    assert(count == count_);
    for (uint32 i = 0; i < count; ++i) {
        if (previous_phase_[i] == PickupPhase::respawning && pickups[i].phase != PickupPhase::respawning) appearance_age_[i] = 0.0F;
        else appearance_age_[i] = fminf(appearance_age_[i] + elapsed_seconds, pickup_appearance_duration);
        previous_phase_[i] = pickups[i].phase;
    }
}

PickupVisual PickupPresentation::visual(const uint32 index, const PickupKind kind, const float presentation_seconds) const {
    assert(initialized_ && index < count_);
    return pickup_visual(index, kind, presentation_seconds, appearance_age_[index]);
}

}
