#pragma once
#include <gloom/core/array.hpp>
#include <gloom/core/fixed_function.hpp>
#include <gloom/core/allocation_profile.hpp>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace gloom::core {
struct JobSystemSettings {
    // Zero selects native logical processor count minus one, with at least one worker.
    uint32 worker_threads{0};
    // Hito 112 startup peak: 102. Saturation assists instead of allocating or dropping work.
    uint32 queue_capacity{256};
};
struct JobSystemMetrics {
    uint64 scheduled_jobs{0};
    uint64 completed_jobs{0};
    uint64 caller_executed_jobs{0};
    uint64 peak_queue_depth{0};
    uint64 job_execution_nanoseconds{0};
    uint64 wait_nanoseconds{0};
    uint32 worker_threads{0};
};
class JobSystem;
// Caller-owned stable address: no pool, allocation or group capacity limit. The system
// outlives groups; drain before destroying one. Never wait on an executing ancestor group.
// Restart invalidates old groups. Join external producers before wait/stop; distinct
// producer threads may schedule into the same live group. Jobs can enqueue descendants.
class TaskGroup {
  public:
    TaskGroup() = default;
    ~TaskGroup() {
        assert(pending_ == 0);
    }
    TaskGroup(const TaskGroup&) = delete;
    TaskGroup& operator=(const TaskGroup&) = delete;
    [[nodiscard]] bool valid() const noexcept {
        return owner_ != nullptr;
    }

  private:
    TaskGroup(JobSystem* owner, uint64 generation) : owner_{owner}, generation_{generation} {}
    JobSystem* owner_{nullptr};
    uint64 generation_{0};
    uint64 pending_{0}; // Protected by owner's lock, including completion and wait predicates.
    friend class JobSystem;
};
class JobSystem {
  public:
    using Job = FixedFunction;
    explicit JobSystem(JobSystemSettings settings = {}) : settings_{settings} {}
    ~JobSystem();
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    // Null means success; otherwise a native thread initialization error. Start/stop are
    // externally serialized. stop drains work and descendants scheduled by executing jobs.
    [[nodiscard]] const char* start();
    void stop() noexcept;
    [[nodiscard]] TaskGroup create_group();
    void schedule(TaskGroup& group, Job job);
    void wait(TaskGroup& group);
    // Named callable and everything it observes live through wait(group). Temporaries
    // deliberately cannot bind: parallel_for enqueues borrowed ranges.
    template <typename Function> void parallel_for(TaskGroup& group, size_t item_count, size_t grain_size, Function& function) {
        assert(grain_size > 0);
        struct RangeJob {
            Function* function;
            size_t begin, end;
            void operator()() noexcept {
                (*function)(begin, end);
            }
        };
        static_assert(noexcept(function(size_t{}, size_t{})), "Parallel-for invocation must not throw");
        for (size_t begin = 0; begin < item_count;) {
            const size_t end = item_count - begin < grain_size ? item_count : begin + grain_size;
            schedule(group, RangeJob{.function = &function, .begin = begin, .end = end});
            begin = end;
        }
    }
    [[nodiscard]] JobSystemMetrics metrics() const noexcept;

  private:
    struct WorkItem {
        TaskGroup* group{nullptr};
        Job job;
    };
    static DWORD WINAPI worker_entry(void* context);
    void worker_loop();
    bool take_locked(WorkItem& item);
    void execute(WorkItem& item, bool assisted);
    void validate_locked(const TaskGroup& group) const;
    JobSystemSettings settings_;
    // ponytail: one protected queue; split queues only if measured contention warrants it.
    mutable SRWLOCK lock_{SRWLOCK_INIT};
    CONDITION_VARIABLE changed_{CONDITION_VARIABLE_INIT};
    CONDITION_VARIABLE work_available_{CONDITION_VARIABLE_INIT};
    Array<WorkItem> queue_;
    Array<HANDLE> workers_;
    size_t head_{0}, queued_{0};
    uint64 active_{0}, generation_{0};
    bool running_{false}, draining_{false}, exiting_{false};
    JobSystemMetrics metrics_;
    char start_error_[128]{};
};
}
