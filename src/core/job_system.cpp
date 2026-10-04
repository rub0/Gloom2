#include <gloom/core/job_system.hpp>
#include <gloom/core/clock.hpp>
#include <stdio.h>

namespace gloom::core {
namespace {
struct Execution {
    JobSystem* system;
    TaskGroup* group;
    Execution* previous;
};
thread_local Execution* executing = nullptr;
bool executing_in(JobSystem* system) {
    for (Execution* frame = executing; frame; frame = frame->previous)
        if (frame->system == system)
            return true;
    return false;
}
uint64 nanoseconds(uint64 ticks) {
    return static_cast<uint64>(ticks * (1000000000.0 / performance_frequency()));
}
}
JobSystem::~JobSystem() {
    stop();
}
const char* JobSystem::start() {
    assert(!running_ && !active_ && !queued_ && settings_.queue_capacity > 0);
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    const uint32 count = settings_.worker_threads ? settings_.worker_threads : info.dwNumberOfProcessors > 1 ? info.dwNumberOfProcessors - 1 : 1;
    queue_.reserve(settings_.queue_capacity);
    queue_.resize(settings_.queue_capacity);
    workers_.reserve(count);
    workers_.resize(0);
    GLOOM_PROFILE_LAYOUT("FixedFunction", sizeof(Job), alignof(Job));
    GLOOM_PROFILE_LAYOUT("WorkItem", sizeof(WorkItem), alignof(WorkItem));
    GLOOM_PROFILE_LAYOUT("TaskGroup", sizeof(TaskGroup), alignof(TaskGroup));
    GLOOM_PROFILE_CAPACITY("JobSystem queue", 0, queue_.size(), sizeof(WorkItem));
    metrics_ = {};
    head_ = 0;
    draining_ = exiting_ = false;
    ++generation_;
    running_ = true;
    for (uint32 index = 0; index < count; ++index) {
        HANDLE worker = CreateThread(nullptr, 0, worker_entry, this, 0, nullptr);
        if (!worker) {
            sprintf_s(start_error_, "JobSystem CreateThread failed (Windows error %lu)", GetLastError());
            stop();
            return start_error_;
        }
        workers_.resize(index + 1);
        workers_[index] = worker;
    }
    metrics_.worker_threads = count;
    return nullptr;
}
TaskGroup JobSystem::create_group() {
    AcquireSRWLockExclusive(&lock_);
    assert(running_ && (!draining_ || executing_in(this)));
    const uint64 generation = generation_;
    ReleaseSRWLockExclusive(&lock_);
    return TaskGroup{this, generation};
}
void JobSystem::validate_locked(const TaskGroup& group) const {
    assert(group.owner_ == this && group.generation_ == generation_);
    static_cast<void>(group);
}
bool JobSystem::take_locked(WorkItem& item) {
    if (!queued_)
        return false;
    item = static_cast<WorkItem&&>(queue_[head_]);
    head_ = (head_ + 1) % queue_.size();
    --queued_;
    ++active_;
    return true;
}
void JobSystem::execute(WorkItem& item, bool assisted) {
#ifdef GLOOM_ALLOCATION_PROFILE
    AllocationScope allocation{AllocationPhase::jobs};
#endif
    const uint64 started = performance_clock();
    Execution frame{.system = this, .group = item.group, .previous = executing};
    executing = &frame;
    item.job();
    // Destruction is part of completion: borrowed contexts may expire as wait returns.
    item.job.reset();
    executing = frame.previous;
#ifdef GLOOM_ALLOCATION_PROFILE
    allocation.finish(); // Publish completion after the diagnostic scope has also drained.
#endif
    AcquireSRWLockExclusive(&lock_);
    metrics_.job_execution_nanoseconds += nanoseconds(performance_clock() - started);
    ++metrics_.completed_jobs;
    if (assisted)
        ++metrics_.caller_executed_jobs;
    --item.group->pending_;
    --active_;
    WakeAllConditionVariable(&changed_);
    ReleaseSRWLockExclusive(&lock_);
    item.group = nullptr;
}
void JobSystem::schedule(TaskGroup& group, Job job) {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::jobs);
    assert(job);
    AcquireSRWLockExclusive(&lock_);
    validate_locked(group);
    assert(running_ && (!draining_ || executing_in(this)));
    while (queued_ == queue_.size()) {
        WorkItem item;
        take_locked(item);
        ReleaseSRWLockExclusive(&lock_);
        execute(item, true);
        AcquireSRWLockExclusive(&lock_);
    }
    ++group.pending_;
    queue_[(head_ + queued_) % queue_.size()] = WorkItem{.group = &group, .job = static_cast<Job&&>(job)};
    ++queued_;
    ++metrics_.scheduled_jobs;
    if (queued_ > metrics_.peak_queue_depth)
        metrics_.peak_queue_depth = queued_;
    // Wake waiters too: nested wait can assist newly published work.
    WakeAllConditionVariable(&changed_);
    WakeConditionVariable(&work_available_);
    ReleaseSRWLockExclusive(&lock_);
}
void JobSystem::wait(TaskGroup& group) {
    for (Execution* frame = executing; frame; frame = frame->previous)
        assert(frame->group != &group);
    const uint64 started = performance_clock();
    AcquireSRWLockExclusive(&lock_);
    validate_locked(group);
    while (group.pending_) {
        WorkItem item;
        if (take_locked(item)) {
            ReleaseSRWLockExclusive(&lock_);
            execute(item, true);
            AcquireSRWLockExclusive(&lock_);
        } else
            SleepConditionVariableSRW(&changed_, &lock_, INFINITE, 0);
    }
    metrics_.wait_nanoseconds += nanoseconds(performance_clock() - started);
    ReleaseSRWLockExclusive(&lock_);
}
DWORD WINAPI JobSystem::worker_entry(void* context) {
    static_cast<JobSystem*>(context)->worker_loop();
    return 0;
}
void JobSystem::worker_loop() {
    AcquireSRWLockExclusive(&lock_);
    for (;;) {
        WorkItem item;
        if (take_locked(item)) {
            ReleaseSRWLockExclusive(&lock_);
            execute(item, false);
            AcquireSRWLockExclusive(&lock_);
        } else if (exiting_) {
            ReleaseSRWLockExclusive(&lock_);
            return;
        } else
            SleepConditionVariableSRW(&work_available_, &lock_, INFINITE, 0);
    }
}
void JobSystem::stop() noexcept {
    assert(!executing_in(this));
    AcquireSRWLockExclusive(&lock_);
    if (!running_) {
        ReleaseSRWLockExclusive(&lock_);
        return;
    }
    draining_ = true;
    while (queued_ || active_) {
        WorkItem item;
        if (take_locked(item)) {
            ReleaseSRWLockExclusive(&lock_);
            execute(item, true);
            AcquireSRWLockExclusive(&lock_);
        } else
            SleepConditionVariableSRW(&changed_, &lock_, INFINITE, 0);
    }
    exiting_ = true;
    WakeAllConditionVariable(&changed_);
    WakeAllConditionVariable(&work_available_);
    ReleaseSRWLockExclusive(&lock_);
    for (HANDLE worker : workers_) {
        WaitForSingleObject(worker, INFINITE);
        CloseHandle(worker);
    }
    workers_.resize(0);
    AcquireSRWLockExclusive(&lock_);
    running_ = false;
    metrics_.worker_threads = 0;
    ReleaseSRWLockExclusive(&lock_);
}
JobSystemMetrics JobSystem::metrics() const noexcept {
    AcquireSRWLockShared(&lock_);
    const JobSystemMetrics result = metrics_;
    ReleaseSRWLockShared(&lock_);
    return result;
}
}
