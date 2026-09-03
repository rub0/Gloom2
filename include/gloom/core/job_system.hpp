#pragma once

#include <gloom/core/subsystem.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace gloom::core {

struct JobSystemSettings {
    // Zero selects hardware_concurrency - 1, with at least one worker.
    std::uint32_t worker_threads{0};
};

struct JobSystemMetrics {
    std::uint64_t scheduled_jobs{0};
    std::uint64_t completed_jobs{0};
    std::uint64_t caller_executed_jobs{0};
    std::uint64_t peak_queue_depth{0};
    std::uint64_t job_execution_nanoseconds{0};
    std::uint64_t wait_nanoseconds{0};
    std::uint32_t worker_threads{0};
};

class TaskGroup final {
public:
    TaskGroup() = default;

    [[nodiscard]] bool valid() const noexcept;

private:
    struct State;
    explicit TaskGroup(std::shared_ptr<State> state);

    std::shared_ptr<State> state_;

    friend class JobSystem;
};

class JobSystem final : public Subsystem {
public:
    using Job = std::move_only_function<void()>;

    explicit JobSystem(JobSystemSettings settings = {});
    ~JobSystem() override;

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    JobSystem(JobSystem&&) = delete;
    JobSystem& operator=(JobSystem&&) = delete;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] SubsystemState state() const noexcept override;
    void start() override;
    void tick(double delta_seconds) override;
    void stop() noexcept override;

    [[nodiscard]] TaskGroup create_group() const;
    void schedule(TaskGroup& group, Job job);
    void wait(TaskGroup& group);

    template <typename Function>
    void parallel_for(TaskGroup& group,
                      const std::size_t item_count,
                      const std::size_t grain_size,
                      Function&& function) {
        if (grain_size == 0) {
            throw std::invalid_argument{"Parallel-for grain size must be greater than zero"};
        }
        using FunctionType = std::decay_t<Function>;
        auto shared_function = std::make_shared<FunctionType>(std::forward<Function>(function));
        for (std::size_t begin = 0; begin < item_count; begin += grain_size) {
            const std::size_t end = item_count - begin < grain_size ? item_count : begin + grain_size;
            schedule(group, [shared_function, begin, end] { (*shared_function)(begin, end); });
        }
    }

    [[nodiscard]] JobSystemMetrics metrics() const noexcept;

private:
    struct Impl;

    JobSystemSettings settings_;
    std::unique_ptr<Impl> impl_;
    SubsystemState state_{SubsystemState::stopped};
};

} // namespace gloom::core
