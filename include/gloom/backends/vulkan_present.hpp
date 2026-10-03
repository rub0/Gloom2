#pragma once
#include <gloom/core/types.hpp>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <time.h>
#endif

namespace gloom {
namespace backends {
// Shared with the pinned Diligent swapchain patch. Modes use VkPresentModeKHR values.
// Single immediate context; measurements are opt-in and in performance-clock ticks.
struct VulkanPresentProfile {
    uint32 unsynced_mode{0};
    uint32 effective_mode{2};
    uint32 image_count{0};
    bool enabled{false};
    uint64 flush{0};
    uint64 queue_present{0};
    uint64 acquire{0};
    uint64 fence_wait{0};
};
extern VulkanPresentProfile vulkan_present;

inline uint64 present_clock() {
    if (!vulkan_present.enabled)
        return 0;
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
}
}
