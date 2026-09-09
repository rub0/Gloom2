#pragma once
#include <gloom/core/types.hpp>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#else
#include <time.h>
#endif

namespace gloom {
// Diagnostic clock; no allocations or output in the measured interval.
inline uint64 performance_clock() {
#ifdef _WIN32
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return static_cast<uint64>(counter.QuadPart);
#else
    timespec counter;
    clock_gettime(CLOCK_MONOTONIC, &counter);
    return static_cast<uint64>(counter.tv_sec) * 1000000000ULL + static_cast<uint64>(counter.tv_nsec);
#endif
}

struct PerformanceProfile {
    uint64 samples[360]{};
    uint64 stages[6]{};
    uint32 count{0};
    uint32 warmup{0};
    uint64 started{performance_clock()};
    uint64 frequency{1000000000ULL};
    PerformanceProfile() {
#ifdef _WIN32
        LARGE_INTEGER clock_frequency;
        QueryPerformanceFrequency(&clock_frequency);
        frequency = static_cast<uint64>(clock_frequency.QuadPart);
#endif
    }
};

inline int compare_performance_samples(const void* left, const void* right) {
    return (*static_cast<const uint64*>(left) > *static_cast<const uint64*>(right)) -
           (*static_cast<const uint64*>(left) < *static_cast<const uint64*>(right));
}

inline void report_performance(PerformanceProfile& profile) {
    double total = 0;
    for (uint32 i = 0; i < profile.count; ++i) total += static_cast<double>(profile.samples[i]);
    qsort(profile.samples, profile.count, sizeof(uint64), compare_performance_samples);
    const double milliseconds = 1000.0 / static_cast<double>(profile.frequency);
    printf("Factory benchmark: samples=%u mean=%.3f ms fps=%.2f p50=%.3f p95=%.3f p99=%.3f max=%.3f ms\n",
        profile.count, total * milliseconds / profile.count, profile.count * 1000.0 / (total * milliseconds),
        profile.samples[profile.count / 2] * milliseconds, profile.samples[(profile.count * 95 - 1) / 100] * milliseconds,
        profile.samples[(profile.count * 99 - 1) / 100] * milliseconds, profile.samples[profile.count - 1] * milliseconds);
    printf("CPU stages ms: update=%.3f begin=%.3f presentation=%.3f visibility=%.3f lighting=%.3f draw_present=%.3f\n",
        profile.stages[0] * milliseconds / profile.count, profile.stages[1] * milliseconds / profile.count,
        profile.stages[2] * milliseconds / profile.count, profile.stages[3] * milliseconds / profile.count,
        profile.stages[4] * milliseconds / profile.count, profile.stages[5] * milliseconds / profile.count);
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX memory{};
    if (K32GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory)))
        printf("Process memory: private=%.2f MiB working_set=%.2f MiB peak_working_set=%.2f MiB\n",
            memory.PrivateUsage / 1048576.0, memory.WorkingSetSize / 1048576.0, memory.PeakWorkingSetSize / 1048576.0);
#endif
}
}
