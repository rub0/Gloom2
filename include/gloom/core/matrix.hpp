#pragma once
#include <stddef.h>

namespace gloom {
// Column-major 4x4 matrix. The serialized/GPU representation is exactly sixteen floats.
struct Matrix4 {
    float values[16]{};
    float& operator[](size_t index) {
        return values[index];
    }
    const float& operator[](size_t index) const {
        return values[index];
    }
    float* data() {
        return values;
    }
    const float* data() const {
        return values;
    }
    float* begin() {
        return values;
    }
    const float* begin() const {
        return values;
    }
    float* end() {
        return values + 16;
    }
    const float* end() const {
        return values + 16;
    }
    bool operator==(const Matrix4&) const = default;
};
static_assert(sizeof(Matrix4) == 16 * sizeof(float));
}
