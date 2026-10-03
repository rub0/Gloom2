#pragma once
#include <gloom/core/types.hpp>
#include <stddef.h>

#ifdef GLOOM_ALLOCATION_PROFILE
namespace gloom {
enum class AllocationPhase : uint32 { other, poses, bounds, jobs, particles, entities, snapshots, ui, assets, renderer, lighting, visibility, count };
struct AllocationStats {
    uint64 calls{0}, bytes{0}, largest{0}, scopes{0}, ticks{0};
};
void allocation_profile_window(bool enabled);
void allocation_profile_reset(); // Requires all measured work drained and the window closed.
AllocationStats allocation_profile_read(AllocationPhase phase);
void allocation_profile_record(size_t bytes);
// Metadata names must have static lifetime. Counts/capacities are independent whole-process maxima.
void allocation_profile_layout(const char* name, size_t size, size_t alignment);
void allocation_profile_capacity(const char* name, size_t count, size_t capacity, size_t element_size);
// Report only after measured scopes/jobs have finished and the window is closed.
void allocation_profile_report(uint32 frames);
// Scopes finish in reverse nesting order on their creating thread; finish may be called early.
struct AllocationScope {
    AllocationPhase previous;
    AllocationPhase phase;
    uint64 started;
    explicit AllocationScope(AllocationPhase value);
    void finish();
    ~AllocationScope();
    AllocationScope(const AllocationScope&) = delete;
    AllocationScope& operator=(const AllocationScope&) = delete;
};
}
#define GLOOM_PROFILE_JOIN_(a, b) a##b
#define GLOOM_PROFILE_JOIN(a, b) GLOOM_PROFILE_JOIN_(a, b)
#define GLOOM_PROFILE_SCOPE(phase)                                                                                                                             \
    ::gloom::AllocationScope GLOOM_PROFILE_JOIN(allocation_scope_, __LINE__) {                                                                                 \
        phase                                                                                                                                                  \
    }
#define GLOOM_PROFILE_LAYOUT(name, size, alignment) ::gloom::allocation_profile_layout(name, size, alignment)
#define GLOOM_PROFILE_CAPACITY(name, count, capacity, size) ::gloom::allocation_profile_capacity(name, count, capacity, size)
#else
#define GLOOM_PROFILE_SCOPE(phase) ((void)0)
#define GLOOM_PROFILE_LAYOUT(name, size, alignment) ((void)0)
#define GLOOM_PROFILE_CAPACITY(name, count, capacity, size) ((void)0)
#endif
