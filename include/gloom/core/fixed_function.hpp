#pragma once
#include <gloom/core/types.hpp>
#include <assert.h>
#include <stddef.h>

namespace gloom {
enum class InlineStorage : uint8 { at };
}
// Tagged placement construction avoids a dependency on the standard-library <new> header.
inline void* operator new(size_t, void* address, gloom::InlineStorage) noexcept {
    return address;
}
inline void operator delete(void*, void*, gloom::InlineStorage) noexcept {}

namespace gloom {
// The scheduler needs only void(). Captures must fit 80 bytes, align to at most 8,
// and be movable, destructible and callable without exceptions. No heap fallback.
class FixedFunction {
  public:
    FixedFunction() = default;
    // Transfer a large, stable callable context to the job. Its destruction is included
    // in completion; cancellation can discard a result without freeing an executing context.
    template <typename Function> explicit FixedFunction(Function* function) noexcept {
        assert(function);
        static_assert(noexcept((*function)()), "Job invocation must not throw");
        static_assert(noexcept(function->~Function()), "Job context destruction must not throw");
        using Pointer = Function*;
        new (storage_, InlineStorage::at) Pointer(function);
        invoke_ = invoke_owned<Function>;
        destroy_ = destroy_owned<Function>;
        move_ = move_owned<Function>;
    }
    template <typename Function> FixedFunction(Function function) noexcept {
        static_assert(sizeof(Function) <= sizeof(storage_), "Job capture exceeds inline storage; use a stable context");
        static_assert(alignof(Function) <= alignof(FixedFunction), "Job capture alignment exceeds inline storage");
        static_assert(noexcept(Function(static_cast<Function&&>(function))), "Job capture move must not throw");
        static_assert(noexcept(function.~Function()), "Job capture destruction must not throw");
        static_assert(noexcept(function()), "Job invocation must not throw");
        new (storage_, InlineStorage::at) Function(static_cast<Function&&>(function));
        invoke_ = invoke<Function>;
        destroy_ = destroy<Function>;
        move_ = move<Function>;
    }
    ~FixedFunction() {
        reset();
    }
    FixedFunction(const FixedFunction&) = delete;
    FixedFunction& operator=(const FixedFunction&) = delete;
    FixedFunction(FixedFunction&& other) noexcept {
        *this = static_cast<FixedFunction&&>(other);
    }
    FixedFunction& operator=(FixedFunction&& other) noexcept {
        if (this == &other)
            return *this;
        reset();
        invoke_ = other.invoke_;
        destroy_ = other.destroy_;
        move_ = other.move_;
        if (move_)
            move_(storage_, other.storage_);
        other.invoke_ = nullptr;
        other.destroy_ = nullptr;
        other.move_ = nullptr;
        return *this;
    }
    explicit operator bool() const noexcept {
        return invoke_ != nullptr;
    }
    void operator()() noexcept {
        assert(invoke_);
        invoke_(storage_);
    }
    void reset() noexcept {
        if (destroy_)
            destroy_(storage_);
        invoke_ = nullptr;
        destroy_ = nullptr;
        move_ = nullptr;
    }

  private:
    template <typename Function> static void invoke_owned(void* storage) noexcept {
        (**static_cast<Function**>(storage))();
    }
    template <typename Function> static void destroy_owned(void* storage) noexcept {
        delete *static_cast<Function**>(storage);
    }
    template <typename Function> static void move_owned(void* destination, void* source) noexcept {
        using Pointer = Function*;
        new (destination, InlineStorage::at) Pointer(*static_cast<Function**>(source));
        *static_cast<Function**>(source) = nullptr;
    }
    template <typename Function> static void invoke(void* storage) noexcept {
        (*static_cast<Function*>(storage))();
    }
    template <typename Function> static void destroy(void* storage) noexcept {
        static_cast<Function*>(storage)->~Function();
    }
    template <typename Function> static void move(void* destination, void* source) noexcept {
        new (destination, InlineStorage::at) Function(static_cast<Function&&>(*static_cast<Function*>(source)));
        destroy<Function>(source);
    }
    alignas(8) uint8 storage_[80]; // Only the placement-constructed callable is read.
    void (*invoke_)(void*) noexcept {nullptr};
    void (*destroy_)(void*) noexcept {nullptr};
    void (*move_)(void*, void*) noexcept {nullptr};
};
}
