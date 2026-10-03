#include <gloom/core/allocation_profile.hpp>
#include <gloom/core/clock.hpp>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace gloom {
namespace {
struct Counters {
    volatile LONG64 calls{0}, bytes{0}, largest{0}, scopes{0}, ticks{0};
};
struct Layout {
    const char* name;
    size_t size, alignment;
};
struct Capacity {
    const char* name;
    size_t count, capacity, element_size;
};
Counters counters[static_cast<uint32>(AllocationPhase::count)];
volatile LONG enabled{0};
thread_local AllocationPhase current{AllocationPhase::other};
SRWLOCK metadata_lock = SRWLOCK_INIT;
Layout layouts[128]{};
Capacity capacities[128]{};
uint32 layout_count{0}, capacity_count{0};
const char* names[]{"other", "poses", "bounds", "jobs", "particles", "entities", "snapshots", "ui", "assets", "renderer", "lighting", "visibility"};

void maximum(volatile LONG64& target, uint64 value) {
    LONG64 old = InterlockedCompareExchange64(&target, 0, 0);
    while (static_cast<uint64>(old) < value) {
        const LONG64 previous = InterlockedCompareExchange64(&target, static_cast<LONG64>(value), old);
        if (previous == old)
            break;
        old = previous;
    }
}
}

void allocation_profile_window(bool value) {
    InterlockedExchange(&enabled, value ? 1 : 0);
}
void allocation_profile_reset() {
    assert(!InterlockedCompareExchange(&enabled, 0, 0));
    for (Counters& value : counters) {
        InterlockedExchange64(&value.calls, 0);
        InterlockedExchange64(&value.bytes, 0);
        InterlockedExchange64(&value.largest, 0);
        InterlockedExchange64(&value.scopes, 0);
        InterlockedExchange64(&value.ticks, 0);
    }
}
AllocationStats allocation_profile_read(AllocationPhase phase) {
    assert(phase < AllocationPhase::count);
    Counters& value = counters[static_cast<uint32>(phase)];
    return {.calls = static_cast<uint64>(InterlockedCompareExchange64(&value.calls, 0, 0)),
        .bytes = static_cast<uint64>(InterlockedCompareExchange64(&value.bytes, 0, 0)),
        .largest = static_cast<uint64>(InterlockedCompareExchange64(&value.largest, 0, 0)),
        .scopes = static_cast<uint64>(InterlockedCompareExchange64(&value.scopes, 0, 0)),
        .ticks = static_cast<uint64>(InterlockedCompareExchange64(&value.ticks, 0, 0))};
}
void allocation_profile_record(size_t bytes) {
    if (!InterlockedCompareExchange(&enabled, 0, 0))
        return;
    Counters& value = counters[static_cast<uint32>(current)];
    InterlockedIncrement64(&value.calls);
    InterlockedAdd64(&value.bytes, static_cast<LONG64>(bytes));
    maximum(value.largest, bytes);
}
AllocationScope::AllocationScope(AllocationPhase value) : previous{current}, phase{value}, started{0} {
    assert(phase < AllocationPhase::count);
    current = phase;
    if (previous != phase && InterlockedCompareExchange(&enabled, 0, 0)) {
        started = performance_clock();
        InterlockedIncrement64(&counters[static_cast<uint32>(phase)].scopes);
    }
}
AllocationScope::~AllocationScope() {
    finish();
}
void AllocationScope::finish() {
    if (phase == AllocationPhase::count)
        return;
    if (started)
        InterlockedAdd64(&counters[static_cast<uint32>(phase)].ticks, static_cast<LONG64>(performance_clock() - started));
    current = previous;
    phase = AllocationPhase::count;
}
void allocation_profile_layout(const char* name, size_t size, size_t alignment) {
    AcquireSRWLockExclusive(&metadata_lock);
    for (uint32 i = 0; i < layout_count; ++i)
        if (layouts[i].size == size && layouts[i].alignment == alignment && strcmp(layouts[i].name, name) == 0) {
            ReleaseSRWLockExclusive(&metadata_lock);
            return;
        }
    assert(layout_count < 128);
    if (layout_count < 128)
        layouts[layout_count++] = {.name = name, .size = size, .alignment = alignment};
    ReleaseSRWLockExclusive(&metadata_lock);
}
void allocation_profile_capacity(const char* name, size_t count, size_t capacity, size_t element_size) {
    AcquireSRWLockExclusive(&metadata_lock);
    for (uint32 i = 0; i < capacity_count; ++i)
        if (strcmp(capacities[i].name, name) == 0) {
            if (count > capacities[i].count)
                capacities[i].count = count;
            if (capacity > capacities[i].capacity)
                capacities[i].capacity = capacity;
            ReleaseSRWLockExclusive(&metadata_lock);
            return;
        }
    assert(capacity_count < 128);
    if (capacity_count < 128)
        capacities[capacity_count++] = {.name = name, .count = count, .capacity = capacity, .element_size = element_size};
    ReleaseSRWLockExclusive(&metadata_lock);
}
void allocation_profile_report(uint32 frames) {
    assert(!InterlockedCompareExchange(&enabled, 0, 0));
    printf("Allocation profile: measured_frames=%u; ordinary new/new[] in linked executable; aligned, DLL and direct C allocations excluded\n", frames);
    for (uint32 i = 0; i < static_cast<uint32>(AllocationPhase::count); ++i) {
        const AllocationStats value = allocation_profile_read(static_cast<AllocationPhase>(i));
        printf("Allocation phase: %s calls=%llu bytes=%llu largest=%llu scopes=%llu cpu_ms=%.6f\n", names[i], value.calls, value.bytes, value.largest,
            value.scopes, value.ticks * 1000.0 / performance_frequency());
    }
    AcquireSRWLockShared(&metadata_lock);
    for (uint32 i = 0; i < layout_count; ++i)
        printf("Allocation layout: size=%zu align=%zu %s\n", layouts[i].size, layouts[i].alignment, layouts[i].name);
    for (uint32 i = 0; i < capacity_count; ++i)
        printf("Allocation capacity: %s peak_count=%zu peak_capacity=%zu element=%zu\n", capacities[i].name, capacities[i].count, capacities[i].capacity,
            capacities[i].element_size);
    ReleaseSRWLockShared(&metadata_lock);
}
}
