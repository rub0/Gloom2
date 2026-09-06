#pragma once

#include <gloom/gameplay/vertical_slice.hpp>
#include <array>

namespace gloom::review {
struct View {
    const char* name;
    float x, y, z, yaw, pitch;
    gameplay::SliceAbility ability{gameplay::SliceAbility::none};
};
inline constexpr std::array views{
    View{"factory", -8.0F, 3.0F, -7.0F, 0.95F, -0.30F},
    View{"weapon-forward", -5.0F, 0.0F, -3.0F, 1.570796327F, 0.0F},
    View{"weapon-yaw", -5.0F, 0.0F, -3.0F, 0.0F, 0.0F},
    View{"weapon-up", -5.0F, 0.0F, -3.0F, 1.570796327F, 0.75F},
    View{"weapon-down", -5.0F, 0.0F, -3.0F, 1.570796327F, -0.75F},
    View{"near-wall", -1.38F, 0.0F, 0.0F, 1.570796327F, 0.0F},
    View{"shadow-detail", -3.4F, 2.0F, -2.8F, 0.85F, -0.65F},
    View{"bite", -5.0F, 0.0F, -3.0F, 1.570796327F, 0.30F, gameplay::SliceAbility::bite},
    View{"guard", -5.0F, 0.0F, -3.0F, 0.70F, -0.30F, gameplay::SliceAbility::guard},
};
inline constexpr std::array factory_views{
    View{"factory-overview", -15.0F, 24.0F, 12.0F, 3.14159265F, -0.70F},
    View{"factory-spawn-1", -35.4855F, 0.10F, 8.9985F, 3.14159265F, -0.06F},
    View{"factory-spawn-8", -21.2694F, 7.3F, -36.007F, 0.40F, -0.14F},
    View{"factory-spawn-9", 13.705F, 7.3F, -35.482F, -1.3F, -0.10F},
    View{"factory-lava", -30.0F, 6.0F, -15.0F, 1.3F, -0.65F},
    View{"factory-central-walkway", -3.0F, 7.0F, -10.0F, -1.5F, -0.3F},
};
inline constexpr std::array pickup_views{
    View{"pickup-available", -55.7487F, .1F, -7.5236F, 3.14159265F, -.30F},
    View{"pickup-pulling", -55.7487F, .1F, -7.5236F, 3.14159265F, -.30F},
    View{"pickup-collected", -55.7487F, .1F, -7.5236F, 3.14159265F, -.30F},
    View{"pickup-respawned", -55.7487F, .1F, -7.5236F, 3.14159265F, -.30F},
};
inline constexpr std::array character_views{
    View{"archangel-front", -35.4855F, 0.10F, 8.9985F, 3.14159265F, -0.18F},
    View{"archangel-back", -35.4855F, 0.10F, 8.9985F, 3.14159265F, -0.18F},
    View{"shadow-front", -35.4855F, 0.10F, 8.9985F, 3.14159265F, -0.18F},
    View{"shadow-back", -35.4855F, 0.10F, 8.9985F, 3.14159265F, -0.18F},
    View{"soul-reaper-forward", -35.4855F, 0.10F, 8.9985F, 3.14159265F, 0.0F},
    View{"soul-reaper-up", -35.4855F, 0.10F, 8.9985F, 3.14159265F, 0.7F},
    View{"soul-reaper-down", -35.4855F, 0.10F, 8.9985F, 3.14159265F, -0.7F},
};
}
