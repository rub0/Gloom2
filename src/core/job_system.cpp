#include <gloom/core/job_system.hpp>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <deque>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace gloom::core {

struct TaskGroup::State {
    explicit State(const JobSystem* group_owner) : owner{group_owner} {}

    const JobSystem* owner;
    std::atomic<std::uint64_t> remaining{0};
    std::mutex mutex;
    std::condition_variable completed;
    std::exception_ptr first_exception;
};

struct JobSystem::Impl {
    struct WorkItem {
        std::shared_ptr<TaskGroup::State> group;
        Job job;
    };

    std::mutex queue_mutex;
    std::condition_variable_any work_available;
    std::condition_variable idle;
    std::deque<WorkItem> queue;
    std::vector<std::jthread> workers;
    bool accepting{false};
    std::uint64_t active_jobs{0};

    std::atomic<std::uint64_t> scheduled_jobs{0};
    std::atomic<std::uint64_t> completed_jobs{0};
    std::atomic<std::uint64_t> caller_executed_jobs{0};
    std::atomic<std::uint64_t> peak_queue_depth{0};
    std::atomic<std::uint64_t> job_execution_nanoseconds{0};
    std::atomic<std::uint64_t> wait_nanoseconds{0};

    void complete(WorkItem& item) noexcept {
        const auto started_at = std::chrono::steady_clock::now();
        try {
            item.job();
        } catch (...) {
            const std::scoped_lock lock{item.group->mutex};
            if (!item.group->first_exception) {
                item.group->first_exception = std::current_exception();
            }
        }

        const auto elapsed = std::chrono::steady_clock::now() - started_at;
        job_execution_nanoseconds.fetch_add(
            static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()),
            std::memory_order_relaxed);

        completed_jobs.fetch_add(1, std::memory_order_relaxed);
        bool group_completed = false;
        {
            // The condition must change while holding the same mutex used by wait().
            // Otherwise a completion can notify between the predicate check and the
            // waiter actually sleeping, leaving a reused task group blocked forever.
            const std::scoped_lock lock{item.group->mutex};
            group_completed =
                item.group->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1;
        }
        if (group_completed) {
            item.group->completed.notify_all();
        }
        {
            const std::scoped_lock lock{queue_mutex};
            --active_jobs;
            if (queue.empty() && active_jobs == 0) {
                idle.notify_all();
            }
        }
    }

    [[nodiscard]] bool take_one(WorkItem& item) {
        const std::scoped_lock lock{queue_mutex};
        if (queue.empty()) {
            return false;
        }
        item = std::move(queue.front());
        queue.pop_front();
        ++active_jobs;
        return true;
    }

    void worker_loop(const std::stop_token stop_token) {
        while (true) {
            WorkItem item;
            {
                std::unique_lock lock{queue_mutex};
                work_available.wait(lock, stop_token, [this] { return !queue.empty(); });
                if (queue.empty()) {
                    if (stop_token.stop_requested()) {
                        return;
                    }
                    continue;
                }
                item = std::move(queue.front());
                queue.pop_front();
                ++active_jobs;
            }
            complete(item);
        }
    }
};

TaskGroup::TaskGroup(std::shared_ptr<State> state) : state_{std::move(state)} {}

bool TaskGroup::valid() const noexcept {
    return static_cast<bool>(state_);
}

JobSystem::JobSystem(const JobSystemSettings settings)
    : settings_{settings}, impl_{std::make_unique<Impl>()} {}

JobSystem::~JobSystem() {
    stop();
}

std::string_view JobSystem::name() const noexcept {
    return "core.jobs";
}

SubsystemState JobSystem::state() const noexcept {
    return state_;
}

void JobSystem::start() {
    if (state_ == SubsystemState::running) {
        throw std::logic_error{"Job system is already running"};
    }
    const auto hardware_threads = std::thread::hardware_concurrency();
    const std::uint32_t automatic_workers = hardware_threads > 1 ? hardware_threads - 1 : 1;
    const std::uint32_t worker_count = settings_.worker_threads == 0
                                           ? automatic_workers
                                           : settings_.worker_threads;
    if (worker_count == 0) {
        throw std::invalid_argument{"Job system requires at least one worker"};
    }

    {
        const std::scoped_lock lock{impl_->queue_mutex};
        impl_->accepting = true;
    }
    impl_->workers.reserve(worker_count);
    for (std::uint32_t index = 0; index < worker_count; ++index) {
        impl_->workers.emplace_back([this](const std::stop_token token) { impl_->worker_loop(token); });
    }
    state_ = SubsystemState::running;
}

void JobSystem::tick([[maybe_unused]] const double delta_seconds) {}

void JobSystem::stop() noexcept {
    if (state_ == SubsystemState::stopped) {
        return;
    }
    {
        std::unique_lock lock{impl_->queue_mutex};
        impl_->accepting = false;
        impl_->idle.wait(lock, [this] { return impl_->queue.empty() && impl_->active_jobs == 0; });
    }
    for (auto& worker : impl_->workers) {
        worker.request_stop();
    }
    impl_->work_available.notify_all();
    impl_->workers.clear();
    state_ = SubsystemState::stopped;
}

TaskGroup JobSystem::create_group() const {
    if (state_ != SubsystemState::running) {
        throw std::logic_error{"Job system must be running before creating a task group"};
    }
    return TaskGroup{std::make_shared<TaskGroup::State>(this)};
}

void JobSystem::schedule(TaskGroup& group, Job job) {
    if (!job) {
        throw std::invalid_argument{"Cannot schedule an empty job"};
    }
    if (!group.state_ || group.state_->owner != this) {
        throw std::invalid_argument{"Task group does not belong to this job system"};
    }

    group.state_->remaining.fetch_add(1, std::memory_order_relaxed);
    try {
        std::uint64_t queue_depth = 0;
        {
            const std::scoped_lock lock{impl_->queue_mutex};
            if (!impl_->accepting) {
                throw std::logic_error{"Job system is not accepting work"};
            }
            impl_->queue.push_back({group.state_, std::move(job)});
            queue_depth = impl_->queue.size();
        }
        impl_->scheduled_jobs.fetch_add(1, std::memory_order_relaxed);
        std::uint64_t peak = impl_->peak_queue_depth.load(std::memory_order_relaxed);
        while (peak < queue_depth &&
               !impl_->peak_queue_depth.compare_exchange_weak(
                   peak, queue_depth, std::memory_order_relaxed)) {
        }
        impl_->work_available.notify_one();
    } catch (...) {
        group.state_->remaining.fetch_sub(1, std::memory_order_relaxed);
        throw;
    }
}

void JobSystem::wait(TaskGroup& group) {
    if (!group.state_ || group.state_->owner != this) {
        throw std::invalid_argument{"Task group does not belong to this job system"};
    }

    const auto wait_started_at = std::chrono::steady_clock::now();
    while (group.state_->remaining.load(std::memory_order_acquire) != 0) {
        Impl::WorkItem item;
        if (impl_->take_one(item)) {
            impl_->caller_executed_jobs.fetch_add(1, std::memory_order_relaxed);
            impl_->complete(item);
            continue;
        }
        std::unique_lock lock{group.state_->mutex};
        group.state_->completed.wait(lock, [&group] {
            return group.state_->remaining.load(std::memory_order_acquire) == 0;
        });
    }
    const auto wait_elapsed = std::chrono::steady_clock::now() - wait_started_at;
    impl_->wait_nanoseconds.fetch_add(
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(wait_elapsed).count()),
        std::memory_order_relaxed);

    std::exception_ptr exception;
    {
        const std::scoped_lock lock{group.state_->mutex};
        exception = group.state_->first_exception;
        group.state_->first_exception = nullptr;
    }
    if (exception) {
        std::rethrow_exception(exception);
    }
}

JobSystemMetrics JobSystem::metrics() const noexcept {
    return {
        .scheduled_jobs = impl_->scheduled_jobs.load(std::memory_order_relaxed),
        .completed_jobs = impl_->completed_jobs.load(std::memory_order_relaxed),
        .caller_executed_jobs = impl_->caller_executed_jobs.load(std::memory_order_relaxed),
        .peak_queue_depth = impl_->peak_queue_depth.load(std::memory_order_relaxed),
        .job_execution_nanoseconds =
            impl_->job_execution_nanoseconds.load(std::memory_order_relaxed),
        .wait_nanoseconds = impl_->wait_nanoseconds.load(std::memory_order_relaxed),
        .worker_threads = static_cast<std::uint32_t>(impl_->workers.size()),
    };
}

} // namespace gloom::core
