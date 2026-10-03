#pragma once

#include <gloom/core/span.hpp>

namespace gloom {

// Owned, reusable storage for default-constructible, move-assignable values. Growth happens only
// in reserve; resizing within capacity never allocates. Views expire on growth.
// ponytail: capacity is default-constructed; use explicit construction if costly defaults become a measured problem.
template <typename T> class Array {
  public:
    Array() = default;
    ~Array() {
        delete[] values_;
    }
    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;

    void reserve(size_t capacity) {
        if (capacity <= capacity_)
            return;
        capacity = capacity > capacity_ * 2 ? capacity : capacity_ * 2;
        T* values = new T[capacity];
        for (size_t index = 0; index < count_; ++index)
            values[index] = static_cast<T&&>(values_[index]);
        delete[] values_;
        values_ = values;
        capacity_ = capacity;
    }
    void resize(size_t count) {
        assert(count <= capacity_);
        for (size_t index = count; index < count_; ++index)
            values_[index] = T{};
        for (size_t index = count_; index < count; ++index)
            values_[index] = T{};
        count_ = count;
    }
    T& operator[](size_t index) {
        assert(index < count_);
        return values_[index];
    }
    const T& operator[](size_t index) const {
        assert(index < count_);
        return values_[index];
    }
    T* data() {
        return values_;
    }
    const T* data() const {
        return values_;
    }
    size_t size() const {
        return count_;
    }
    size_t capacity() const {
        return capacity_;
    }
    T* begin() {
        return values_;
    }
    const T* begin() const {
        return values_;
    }
    T* end() {
        return count_ ? values_ + count_ : values_;
    }
    const T* end() const {
        return count_ ? values_ + count_ : values_;
    }

  private:
    T* values_{nullptr};
    size_t count_{0};
    size_t capacity_{0};
};

} // namespace gloom
