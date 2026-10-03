#include <gloom/core/clock.hpp>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

namespace gloom {
uint64 performance_clock() noexcept {
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
uint64 performance_frequency() noexcept {
#ifdef _WIN32
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return static_cast<uint64>(frequency.QuadPart);
#else
    return 1000000000ULL;
#endif
}
} // namespace gloom
