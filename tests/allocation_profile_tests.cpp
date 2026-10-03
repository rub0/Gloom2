#include <gloom/core/allocation_profile.hpp>
#include <gloom/core/array.hpp>
#include <stdio.h>
#include <stdlib.h>
#define NOMINMAX
#include <windows.h>

using namespace gloom;
namespace {
void require(bool value, const char* message) {
    if (!value) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}
DWORD WINAPI allocations_worker(void*) {
    GLOOM_PROFILE_SCOPE(AllocationPhase::jobs);
    GLOOM_PROFILE_LAYOUT("worker uint32", sizeof(uint32), alignof(uint32));
    for (uint32 i = 0; i < 1000; ++i) {
        void* value = ::operator new(17);
        ::operator delete(value);
        void* array = ::operator new[](33);
        ::operator delete[](array);
    }
    return 0;
}
}
int main() {
    allocation_profile_reset();
    allocations_worker(nullptr);
    require(allocation_profile_read(AllocationPhase::jobs).calls == 0, "Closed window counted allocations");
    allocation_profile_window(true);
    HANDLE workers[4]{};
    for (HANDLE& worker : workers) {
        worker = CreateThread(nullptr, 0, allocations_worker, nullptr, 0, nullptr);
        require(worker != nullptr, "Worker initialization failed");
    }
    WaitForMultipleObjects(4, workers, TRUE, INFINITE);
    for (HANDLE worker : workers)
        CloseHandle(worker);
    allocation_profile_window(false);
    const AllocationStats jobs = allocation_profile_read(AllocationPhase::jobs);
    require(jobs.calls == 8000 && jobs.bytes == 200000 && jobs.largest == 33 && jobs.scopes == 4, "Concurrent allocation counts or TLS attribution lost");
    require(allocation_profile_read(AllocationPhase::poses).calls == 0, "Worker phase leaked to another phase");
    allocation_profile_reset();
    Array<uint32> storage;
    storage.reserve(32);
    allocation_profile_window(true);
    {
        GLOOM_PROFILE_SCOPE(AllocationPhase::poses);
        for (uint32 i = 0; i < 1000; ++i)
            storage.resize(32);
        require(allocation_profile_read(AllocationPhase::poses).calls == 0, "Warmed storage allocated");
        storage.reserve(64);
    }
    allocation_profile_window(false);
    require(allocation_profile_read(AllocationPhase::poses).calls == 1, "Growth control was not detected");
    require(allocation_profile_read(AllocationPhase::poses).bytes == 64 * sizeof(uint32), "Requested bytes incorrect");
    allocation_profile_window(true);
    {
        AllocationScope scope{AllocationPhase::ui};
        scope.finish();
        scope.finish();
        allocation_profile_record(7);
    }
    allocation_profile_window(false);
    require(allocation_profile_read(AllocationPhase::ui).calls == 0 && allocation_profile_read(AllocationPhase::other).bytes == 7,
        "Finished scope did not restore attribution");
    allocation_profile_report(0);
    puts("Allocation profile controls passed: closed window, 4 workers, 8000 calls, warmed storage and growth.");
    return 0;
}
