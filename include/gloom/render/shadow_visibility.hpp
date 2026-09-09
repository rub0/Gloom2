#pragma once
#include <gloom/render/scene.hpp>
#include <gloom/core/types.hpp>
#include <math.h>

namespace gloom::render {
// Row-major local-to-clip matrix, row vectors, Vulkan depth [0,w].
// Conservative sphere/plane rejection also handles nonuniform object scale.
inline bool shadow_visible(const BoundingSphere& bounds, const float* matrix) {
    for (uint32 plane = 0; plane < 6; ++plane) {
        const uint32 axis = plane / 2;
        const float sign = (plane & 1) ? -1.0F : 1.0F;
        const float w = plane == 4 ? 0.0F : 1.0F;
        const float x = w * matrix[3] + sign * matrix[axis];
        const float y = w * matrix[7] + sign * matrix[4 + axis];
        const float z = w * matrix[11] + sign * matrix[8 + axis];
        const float distance = bounds.center.x * x + bounds.center.y * y + bounds.center.z * z +
            w * matrix[15] + sign * matrix[12 + axis];
        if (distance < -bounds.radius * sqrtf(x * x + y * y + z * z) - 0.0001F) return false;
    }
    return true;
}
}
