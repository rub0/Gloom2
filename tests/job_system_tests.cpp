#include <gloom/core/job_system.hpp>
#include <gloom/core/clock.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

using namespace gloom;
using namespace gloom::core;
namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "Job-system test failure: %s\n", message);
        exit(1);
    }
}
struct Increment {
    volatile LONG* count;
    uint32 amount{1};
    void operator()() const noexcept {
        InterlockedExchangeAdd(count, amount);
    }
};
struct Sum {
    volatile LONG64* value;
    void operator()(size_t begin, size_t end) const noexcept {
        uint64 sum = 0;
        for (size_t index = begin; index < end; ++index)
            sum += index;
        InterlockedExchangeAdd64(value, sum);
    }
};
struct Nested {
    JobSystem* jobs;
    volatile LONG* count;
    void operator()() const noexcept {
        TaskGroup inner = jobs->create_group();
        for (uint32 index = 0; index < 64; ++index)
            jobs->schedule(inner, Increment{.count = count});
        jobs->wait(inner);
    }
};
struct Resource {
    volatile LONG* executed;
    volatile LONG* destroyed;
    uint32* value;
    Resource(volatile LONG* executions, volatile LONG* destructions) : executed{executions}, destroyed{destructions}, value{new uint32{77}} {}
    Resource(const Resource&) = delete;
    Resource(Resource&& other) noexcept : executed{other.executed}, destroyed{other.destroyed}, value{other.value} {
        other.value = nullptr;
    }
    ~Resource() {
        if (value) {
            require(*value == 77, "Movable capture data corrupted");
            delete value;
            InterlockedIncrement(destroyed);
        }
    }
    void operator()() const noexcept {
        require(value && *value == 77, "Capture died before invocation");
        InterlockedIncrement(executed);
    }
};
struct ExplicitFailure {
    int32* result;
    volatile LONG* completed;
    void operator()() const noexcept {
        *result = 42;
        InterlockedIncrement(completed);
    }
};
struct Producer {
    JobSystem* jobs;
    TaskGroup* group;
    volatile LONG* counts;
    uint32 begin, end;
};
DWORD WINAPI produce(void* data) {
    Producer& producer = *static_cast<Producer*>(data);
    for (uint32 index = producer.begin; index < producer.end; ++index)
        producer.jobs->schedule(*producer.group, Increment{.count = &producer.counts[index]});
    return 0;
}
int compare_ticks(const void* a, const void* b) {
    return *static_cast<const uint64*>(a) < *static_cast<const uint64*>(b) ? -1 : *static_cast<const uint64*>(a) > *static_cast<const uint64*>(b) ? 1 : 0;
}
void exercise(uint32 workers) {
    JobSystem jobs{{.worker_threads = workers, .queue_capacity = 16}};
    require(jobs.start() == nullptr, "Native thread initialization failed");
    {
        TaskGroup empty = jobs.create_group();
        jobs.wait(empty);
        jobs.wait(empty);
        volatile LONG64 sum = 0;
        Sum ranges{.value = &sum};
        jobs.parallel_for(empty, 0, 97, ranges);
        jobs.wait(empty);
        jobs.parallel_for(empty, 10000, 97, ranges);
        jobs.wait(empty);
        require(sum == 49995000, "Parallel-for lost or duplicated work");
        volatile LONG nested_count = 0;
        jobs.schedule(empty, Nested{.jobs = &jobs, .count = &nested_count});
        jobs.wait(empty);
        require(nested_count == 64, "Nested wait lost work");

        volatile LONG executed = 0, destroyed = 0;
        FixedFunction first{Resource{&executed, &destroyed}};
        FixedFunction second{static_cast<FixedFunction&&>(first)};
        require(!first && second, "Move did not clear the source");
        second = static_cast<FixedFunction&&>(second);
        FixedFunction replaced{Resource{&executed, &destroyed}};
        replaced = static_cast<FixedFunction&&>(second);
        require(!second && destroyed == 1, "Move assignment did not destroy replaced capture");
        jobs.schedule(empty, static_cast<FixedFunction&&>(replaced));
        jobs.wait(empty);
        require(!replaced && executed == 1 && destroyed == 2, "Capture destruction must finish before wait returns");
        FixedFunction heap{new Resource{&executed, &destroyed}};
        FixedFunction moved_heap{static_cast<FixedFunction&&>(heap)};
        jobs.schedule(empty, static_cast<FixedFunction&&>(moved_heap));
        jobs.wait(empty);
        require(executed == 2 && destroyed == 3, "Owned context was lost or destroyed twice");
        int32 error = 0;
        jobs.schedule(empty, ExplicitFailure{.result = &error, .completed = &executed});
        jobs.schedule(empty, Increment{.count = &executed});
        jobs.wait(empty);
        require(error == 42 && executed == 4, "Explicit failure prevented other jobs completing");
        for (uint32 index = 0; index < 128; ++index)
            jobs.schedule(empty, Resource{&executed, &destroyed});
        jobs.wait(empty);
        require(executed == 132 && destroyed == 131, "Queued movable captures were lost or destroyed twice");
        struct Boundary {
            uint64 payload[9];
            volatile LONG* count;
            void operator()() const noexcept {
                require(payload[8] == 77, "Inline boundary data corrupted");
                InterlockedIncrement(count);
            }
        };
        static_assert(sizeof(Boundary) == 80 && alignof(Boundary) == 8);
        FixedFunction boundary{Boundary{.payload = {0, 0, 0, 0, 0, 0, 0, 0, 77}, .count = &executed}};
        jobs.schedule(empty, static_cast<FixedFunction&&>(boundary));
        jobs.wait(empty);
        require(executed == 133 && !boundary, "Maximum inline capture did not survive movement");
        FixedFunction vacant;
        vacant.reset();
        require(!vacant, "Empty callable reports valid");

        // Every index is checked: a correct total cannot hide one lost and one duplicate job.
        volatile LONG counts[4096]{};
        Producer producers[4];
        HANDLE threads[4];
        for (uint32 index = 0; index < 4; ++index) {
            producers[index] = {.jobs = &jobs, .group = &empty, .counts = counts, .begin = index * 1024, .end = (index + 1) * 1024};
            threads[index] = CreateThread(nullptr, 0, produce, &producers[index], 0, nullptr);
            require(threads[index] != nullptr, "Producer thread initialization failed");
        }
        for (HANDLE thread : threads) {
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
        }
        jobs.wait(empty);
        for (LONG count : counts)
            require(count == 1, "Concurrent producers lost or duplicated a job");

        // Force saturation independently of timing: block every worker, then fill all slots.
        HANDLE entered = CreateSemaphoreW(nullptr, 0, workers, nullptr);
        HANDLE released = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        require(entered && released, "Gate initialization failed");
        struct WorkerGate {
            HANDLE entered, released;
            void operator()() const noexcept {
                ReleaseSemaphore(entered, 1, nullptr);
                WaitForSingleObject(released, INFINITE);
            }
        };
        for (uint32 index = 0; index < workers; ++index)
            jobs.schedule(empty, WorkerGate{.entered = entered, .released = released});
        for (uint32 index = 0; index < workers; ++index)
            require(WaitForSingleObject(entered, 5000) == WAIT_OBJECT_0, "Worker never entered gate");
        volatile LONG saturated = 0;
        uint64 helped = jobs.metrics().caller_executed_jobs;
        for (uint32 index = 0; index < 257; ++index)
            jobs.schedule(empty, Increment{.count = &saturated});
        require(jobs.metrics().peak_queue_depth == 16 && jobs.metrics().caller_executed_jobs > helped, "Full queue did not assist within capacity");
        SetEvent(released);
        jobs.wait(empty);
        CloseHandle(entered);
        CloseHandle(released);
        require(saturated == 257, "Saturation lost work");

        // Zero-own-new window includes group creation, scheduling, execution and destruction.
        volatile LONG reused = 0;
        uint64 ticks[1000];
        for (uint32 index = 0; index < 3; ++index)
            jobs.schedule(empty, Increment{.count = &reused});
        jobs.wait(empty);
        const JobSystemMetrics initial = jobs.metrics();
#ifdef GLOOM_ALLOCATION_PROFILE
        allocation_profile_window(false);
        allocation_profile_reset();
        allocation_profile_window(true);
#endif
        for (uint32 round = 0; round < 1000; ++round) {
            uint64 started = performance_clock();
            TaskGroup local = jobs.create_group();
            for (uint32 index = 0; index < 3; ++index)
                jobs.schedule(local, Increment{.count = &reused});
            jobs.wait(local);
            for (uint32 index = 0; index < 3; ++index)
                jobs.schedule(empty, Increment{.count = &reused});
            jobs.wait(empty);
            ticks[round] = performance_clock() - started;
        }
#ifdef GLOOM_ALLOCATION_PROFILE
        allocation_profile_window(false);
        uint64 calls = 0, bytes = 0;
        for (uint32 phase = 0; phase < static_cast<uint32>(AllocationPhase::count); ++phase) {
            AllocationStats stats = allocation_profile_read(static_cast<AllocationPhase>(phase));
            calls += stats.calls;
            bytes += stats.bytes;
        }
        printf("Job allocation window: workers=%u rounds=1000 jobs=6000 own_new=%llu bytes=%llu\n", workers, calls, bytes);
        require(calls == 0 && bytes == 0, "Warm schedule/wait allocated");
#endif
        require(reused == 6003, "Repeated groups lost work or completion notification");
        qsort(ticks, 1000, sizeof(uint64), compare_ticks);
        JobSystemMetrics metrics = jobs.metrics();
        printf("Job batch: workers=%u rounds=1000 median_us=%.3f p95_us=%.3f peak=%llu assisted=%llu scheduled=%llu completed=%llu wait_ms=%.3f\n", workers,
            ticks[500] * 1000000.0 / performance_frequency(), ticks[950] * 1000000.0 / performance_frequency(), metrics.peak_queue_depth,
            metrics.caller_executed_jobs - initial.caller_executed_jobs, metrics.scheduled_jobs - initial.scheduled_jobs,
            metrics.completed_jobs - initial.completed_jobs, (metrics.wait_nanoseconds - initial.wait_nanoseconds) / 1000000.0);
        require(metrics.worker_threads == workers && metrics.scheduled_jobs == metrics.completed_jobs && metrics.job_execution_nanoseconds &&
                    metrics.wait_nanoseconds,
            "Metrics did not cover all completed jobs");
        // stop must drain descendants, not merely wait for the initial queue to empty.
        jobs.schedule(empty, Nested{.jobs = &jobs, .count = &nested_count});
        jobs.stop();
        require(nested_count == 128 && jobs.metrics().scheduled_jobs == jobs.metrics().completed_jobs, "stop did not drain descendants");
        jobs.wait(empty);
        jobs.stop();
    }
    require(jobs.start() == nullptr, "Restart failed");
    {
        TaskGroup group = jobs.create_group();
        volatile LONG count = 0;
        for (uint32 index = 0; index < 123; ++index)
            jobs.schedule(group, Increment{.count = &count});
        jobs.stop();
        require(count == 123, "Restart/stop lost pending jobs");
    }
}
}
int main(int argc, const char* const* argv) {
#ifdef _DEBUG
    if (argc == 2) {
        _set_error_mode(_OUT_TO_STDERR);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        JobSystem jobs{{.worker_threads = 1}};
        require(jobs.start() == nullptr, "Assertion fixture initialization failed");
        TaskGroup group = jobs.create_group();
        volatile LONG count = 0;
        if (strcmp(argv[1], "foreign") == 0) {
            JobSystem other{{.worker_threads = 1}};
            require(other.start() == nullptr, "Foreign fixture initialization failed");
            other.schedule(group, Increment{.count = &count});
        } else if (strcmp(argv[1], "stale") == 0) {
            jobs.stop();
            require(jobs.start() == nullptr, "Stale fixture restart failed");
            jobs.schedule(group, Increment{.count = &count});
        } else if (strcmp(argv[1], "same") == 0) {
            struct SameGroup {
                JobSystem* jobs;
                TaskGroup* group;
                void operator()() const noexcept {
                    jobs->wait(*group);
                }
            };
            jobs.schedule(group, SameGroup{.jobs = &jobs, .group = &group});
            jobs.wait(group);
        }
        return 0;
    }
    // Programmer misuse must assert promptly rather than becoming an infinite wait.
    char executable[MAX_PATH];
    require(GetModuleFileNameA(nullptr, executable, MAX_PATH) > 0, "Could not locate assertion fixture");
    const char* modes[]{"foreign", "stale", "same"};
    for (const char* mode : modes) {
        char command[MAX_PATH + 32];
        snprintf(command, sizeof(command), "\"%s\" %s", executable, mode);
        STARTUPINFOA startup{.cb = sizeof(startup)};
        PROCESS_INFORMATION process{};
        require(CreateProcessA(nullptr, command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process),
            "Could not start assertion fixture");
        require(WaitForSingleObject(process.hProcess, 5000) == WAIT_OBJECT_0, "Programmer misuse hung instead of asserting");
        DWORD result = 0;
        GetExitCodeProcess(process.hProcess, &result);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        require(result == 3, "Expected programmer assertion was not enforced");
    }
#else
    static_cast<void>(argc);
    static_cast<void>(argv);
#endif
    exercise(1);
    exercise(4);
    puts("Gloom job-system tests completed successfully.");
    return 0;
}
