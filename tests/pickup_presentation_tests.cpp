#include <gloom/gameplay/pickup_presentation.hpp>

#include <math.h>
#include <stdio.h>

namespace {
int failures;
void require(const bool condition, const char* const message) {
    if (!condition) {
        ++failures;
        fprintf(stderr, "%s\n", message);
    }
}
}

int main() {
    using namespace gloom::gameplay;
    const PickupVisual first = pickup_visual(0, PickupKind::shield, 0.0F, pickup_appearance_duration);
    const PickupVisual later = pickup_visual(0, PickupKind::shield, 1.0F, pickup_appearance_duration);
    require(fabsf(first.vertical_offset) <= 0.0801F && fabsf(later.vertical_offset) <= 0.0801F, "Bobbing escaped its bounds");
    require(later.yaw > first.yaw, "Pickup yaw did not advance");
    require(first.scale == 1.0F && first.opacity == 1.0F && first.halo_alpha > 0.0F, "Available pickup presentation differs");
    require(pickup_visual(1, PickupKind::life, 2.0F, pickup_appearance_duration).halo_alpha == 0.0F, "Life pickup has a halo");

    PickupView initial_view;
    PickupPresentation initial_presentation;
    initial_presentation.update(&initial_view, 1, 0.0F);
    PickupVisual appeared = initial_presentation.visual(0, PickupKind::shield, 0.0F);
    require(appeared.scale == 0.35F && appeared.opacity == 0.0F, "Initial appearance did not start hidden and small");
    initial_presentation.update(&initial_view, 1, pickup_appearance_duration);
    appeared = initial_presentation.visual(0, PickupKind::shield, 0.0F);
    require(appeared.scale == 1.0F && appeared.opacity == 1.0F, "Initial appearance did not finish fully visible");

    PickupView view{.phase = PickupPhase::respawning};
    PickupPresentation presentation;
    presentation.update(&view, 1, 0.0F);
    view.phase = PickupPhase::available;
    presentation.update(&view, 1, 0.0F);
    appeared = presentation.visual(0, PickupKind::shield, 0.0F);
    require(appeared.scale == 0.35F && appeared.opacity == 0.0F && appeared.halo_alpha == 0.0F, "Respawn did not start hidden and small");
    presentation.update(&view, 1, pickup_appearance_duration * 0.5F);
    appeared = presentation.visual(0, PickupKind::shield, 0.0F);
    require(appeared.scale > 0.35F && appeared.scale < 1.0F && appeared.opacity > 0.0F && appeared.opacity < 1.0F, "Respawn did not interpolate");
    presentation.update(&view, 1, pickup_appearance_duration);
    appeared = presentation.visual(0, PickupKind::shield, 0.0F);
    require(appeared.scale == 1.0F && appeared.opacity == 1.0F, "Respawn did not finish fully visible");

    const PickupVisual phase_a = pickup_visual(2, PickupKind::ammo, 3.0F, pickup_appearance_duration);
    const PickupVisual phase_b = pickup_visual(3, PickupKind::ammo, 3.0F, pickup_appearance_duration);
    require(fabsf(phase_a.vertical_offset - phase_b.vertical_offset) > 0.001F, "Pickup phases are synchronized");
    if (!failures) printf("Pickup bobbing, yaw, halo exclusion and respawn presentation passed\n");
    return failures ? 1 : 0;
}
