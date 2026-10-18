#pragma once

#include "task/task_scheduler.hpp"
#include <catch2/catch_all.hpp>

namespace violet::test
{
constexpr std::size_t NUM_THREAD = 4;
constexpr std::size_t NUM_DATA_PER_THREAD = 10000;

/**
 * @brief Runs the shared scheduler for the lifetime of one test case.
 *
 * The scheduler is a singleton, so every test case that touches it must start from a
 * clean run() and finish with stop(). Doing that through RAII also covers the case where
 * an assertion throws: the next test case would otherwise call run() while the scheduler
 * is still running and trip its "not running" assertion.
 */
class scheduler_scope
{
public:
    explicit scheduler_scope(std::uint32_t worker_count)
        : m_scheduler(task_scheduler::instance())
    {
        m_scheduler.run(worker_count);
    }

    ~scheduler_scope()
    {
        m_scheduler.stop();
    }

    scheduler_scope(const scheduler_scope&) = delete;
    scheduler_scope& operator=(const scheduler_scope&) = delete;

    task_scheduler& get() noexcept
    {
        return m_scheduler;
    }

private:
    task_scheduler& m_scheduler;
};
} // namespace violet::test