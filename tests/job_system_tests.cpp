#include <gloom/core/job_system.hpp>

#include <atomic>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

} // namespace

int main() try {
    gloom::core::JobSystem jobs{gloom::core::JobSystemSettings{.worker_threads = 4}};
    jobs.start();

    auto parallel_group = jobs.create_group();
    std::atomic<std::uint64_t> sum{0};
    constexpr std::size_t item_count = 10'000;
    jobs.parallel_for(parallel_group, item_count, 97, [&](const std::size_t begin, const std::size_t end) {
        for (std::size_t index = begin; index < end; ++index) {
            sum.fetch_add(index, std::memory_order_relaxed);
        }
    });
    jobs.wait(parallel_group);
    constexpr std::uint64_t expected_sum = item_count * (item_count - 1) / 2;
    expect(sum.load() == expected_sum, "Parallel-for lost or duplicated work");

    auto nested_group = jobs.create_group();
    std::atomic<std::uint32_t> nested_count{0};
    jobs.schedule(nested_group, [&] {
        auto inner_group = jobs.create_group();
        for (int index = 0; index < 64; ++index) {
            jobs.schedule(inner_group, [&] { nested_count.fetch_add(1, std::memory_order_relaxed); });
        }
        jobs.wait(inner_group);
    });
    jobs.wait(nested_group);
    expect(nested_count.load() == 64, "Nested task-group wait deadlocked or lost work");

    auto exception_group = jobs.create_group();
    jobs.schedule(exception_group, [] { throw std::runtime_error{"expected job failure"}; });
    bool exception_observed = false;
    try {
        jobs.wait(exception_group);
    } catch (const std::runtime_error&) {
        exception_observed = true;
    }
    expect(exception_observed, "Job exception was not propagated to the waiting thread");

    auto reused_group = jobs.create_group();
    std::atomic<std::uint32_t> reuse_count{0};
    constexpr std::uint32_t reuse_rounds = 1'000;
    for (std::uint32_t round = 0; round < reuse_rounds; ++round) {
        for (std::uint32_t job = 0; job < 3; ++job) {
            jobs.schedule(reused_group,
                          [&] { reuse_count.fetch_add(1, std::memory_order_relaxed); });
        }
        jobs.wait(reused_group);
    }
    expect(reuse_count.load() == reuse_rounds * 3,
           "Reused task group lost work or a completion notification");

    const auto metrics = jobs.metrics();
    expect(metrics.worker_threads == 4, "Requested worker count was not used");
    expect(metrics.scheduled_jobs == metrics.completed_jobs, "Not all scheduled jobs completed");
    expect(metrics.peak_queue_depth > 0, "Queue metrics were not recorded");
    expect(metrics.job_execution_nanoseconds > 0, "Job execution time was not recorded");
    expect(metrics.wait_nanoseconds > 0, "Task-group wait time was not recorded");
    jobs.stop();

    std::cout << "Gloom job-system tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Job-system test failure: " << error.what() << '\n';
    return 1;
}
