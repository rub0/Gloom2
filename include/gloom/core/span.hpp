#pragma once

#include <gloom/core/types.hpp>
#include <assert.h>
#include <stddef.h>
#include <initializer_list>

namespace gloom {

// Non-owning view. Initializer-list storage lasts only through the enclosing call.
template <typename T> struct Span {
    T* values{nullptr};
    size_t count{0};

    constexpr Span() = default;
    constexpr Span(T* data, size_t size) : values{data}, count{size} {
        assert(data != nullptr || size == 0);
    }
    template <size_t N> constexpr Span(T (&data)[N]) : values{data}, count{N} {}
    template <typename U> constexpr Span(Span<U> data) : values{data.values}, count{data.count} {}
    constexpr Span(std::initializer_list<T> data) : values{data.begin()}, count{data.size()} {}

    constexpr T& operator[](size_t index) const {
        assert(index < count);
        return values[index];
    }
    constexpr T* begin() const {
        return values;
    }
    constexpr T* data() const {
        return values;
    }
    constexpr T* end() const {
        return count ? values + count : values;
    }
    constexpr size_t size() const {
        return count;
    }
    constexpr bool empty() const {
        return count == 0;
    }
};

} // namespace gloom
