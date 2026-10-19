#include "test_task_common.hpp"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

namespace violet::test
{
TEST_CASE("task_scheduler: run and stop", "[task2][scheduler]")
{
    auto& scheduler = task_scheduler::instance();

    scheduler.run(2);
    CHECK(scheduler.get_thread_type() == TASK_THREAD_MAIN);

    scheduler.stop();
    scheduler.stop(); // stopping twice must be a no-op
}

TEST_CASE("task_scheduler: dispatch runs an any-affinity task", "[task2][scheduler]")
{
    scheduler_scope scope(3);
    task_scheduler& scheduler = scope.get();

    const std::thread::id main_id = std::this_thread::get_id();
    std::atomic<std::int32_t> executed{0};

    task_handle handle = scheduler.dispatch(
        [&]()
        {
            executed.fetch_add(1);
        });

    REQUIRE(handle.is_valid());
    handle.wait();

    CHECK(executed.load() == 1);
    CHECK(handle.is_complete());
}

TEST_CASE("task_scheduler: every dispatched task runs exactly once", "[task2][scheduler]")
{
    constexpr std::size_t task_count = 512;

    scheduler_scope scope(4);
    task_scheduler& scheduler = scope.get();

    auto seen = std::make_unique<std::atomic<std::int32_t>[]>(task_count);
    for (std::size_t i = 0; i < task_count; ++i)
    {
        seen[i].store(0);
    }

    std::vector<task_handle> handles;
    handles.reserve(task_count);

    for (std::size_t i = 0; i < task_count; ++i)
    {
        handles.push_back(scheduler.dispatch(
            [&seen, i]()
            {
                seen[i].fetch_add(1);
            }));
    }

    for (task_handle& handle : handles)
    {
        handle.wait();
    }

    std::int32_t total = 0;
    std::int32_t wrong = 0;

    for (std::size_t i = 0; i < task_count; ++i)
    {
        const std::int32_t count = seen[i].load();
        total += count;

        if (count != 1)
        {
            ++wrong;
        }
    }

    CHECK(total == static_cast<std::int32_t>(task_count));
    CHECK(wrong == 0);
}

TEST_CASE("task_scheduler: main-affinity tasks only run on pump_main", "[task2][scheduler]")
{
    scheduler_scope scope(3);
    task_scheduler& scheduler = scope.get();

    std::atomic<std::int32_t> executed{0};

    task_handle handle = scheduler.dispatch(
        [&]()
        {
            executed.fetch_add(1);
        },
        TASK_AFFINITY_MAIN);

    // Only the main lane drains the main queue, and nothing has pumped yet.
    CHECK(executed.load() == 0);

    CHECK(scheduler.pump_main(64) >= 1u);
    CHECK(executed.load() == 1);
    CHECK(handle.is_complete());

    handle.wait(); // already complete, must return immediately
    CHECK(executed.load() == 1);
}

TEST_CASE("task_scheduler: pump_main honours max_count and drains the queue", "[task2][scheduler]")
{
    constexpr std::int32_t task_count = 8;

    scheduler_scope scope(2);
    task_scheduler& scheduler = scope.get();

    std::atomic<std::int32_t> executed{0};

    for (std::int32_t i = 0; i < task_count; ++i)
    {
        scheduler.dispatch(
            [&]()
            {
                executed.fetch_add(1);
            },
            TASK_AFFINITY_MAIN);
    }

    CHECK(executed.load() == 0);

    // budget <= 0 disables the time limit, so max_count is the only bound.
    CHECK(scheduler.pump_main(3, std::chrono::microseconds(0)) == 3u);
    CHECK(executed.load() == 3);

    CHECK(scheduler.pump_main(64) == static_cast<std::size_t>(task_count - 3));
    CHECK(executed.load() == task_count);

    CHECK(scheduler.pump_main(64) == 0u); // empty queue
}

TEST_CASE("task_scheduler: blocking tasks never run on the main thread", "[task2][scheduler]")
{
    scheduler_scope scope(3); // two compute workers plus one io worker
    task_scheduler& scheduler = scope.get();

    const std::thread::id main_id = std::this_thread::get_id();
    std::atomic<bool> executed{false};
    std::atomic<bool> ran_on_main{false};

    task_handle handle = scheduler.dispatch(
        [&]()
        {
            if (std::this_thread::get_id() == main_id)
            {
                ran_on_main.store(true);
            }

            executed.store(true);
        },
        TASK_AFFINITY_BLOCKING);

    handle.wait();

    CHECK(executed.load());
    CHECK_FALSE(ran_on_main.load());
}

TEST_CASE("task_scheduler: single worker configuration", "[task2][scheduler]")
{
    scheduler_scope scope(1);
    task_scheduler& scheduler = scope.get();

    std::atomic<std::int32_t> executed{0};

    task_handle any = scheduler.dispatch(
        [&]()
        {
            executed.fetch_add(1);
        });

    task_handle on_main = scheduler.dispatch(
        [&]()
        {
            executed.fetch_add(1);
        },
        TASK_AFFINITY_MAIN);

    any.wait();
    scheduler.pump_main(8);
    on_main.wait();

    // The blocking lane is deliberately not covered here: run(1) leaves m_io_queue from a
    // previous run(n > 1) untouched, so a blocking task dispatched in this configuration
    // would be queued where no thread consumes it.
    CHECK(executed.load() == 2);
}

TEST_CASE("task_scheduler: a task can dispatch and wait for a child task", "[task2][scheduler]")
{
    scheduler_scope scope(2);
    task_scheduler& scheduler = scope.get();

    std::atomic<std::int32_t> child_executed{0};
    std::atomic<std::int32_t> parent_executed{0};

    // The child lands in the parent's local queue. The waiting parent has to run it
    // itself, otherwise this deadlocks.
    task_handle parent = scheduler.dispatch(
        [&]()
        {
            task_handle child = scheduler.dispatch(
                [&]()
                {
                    child_executed.fetch_add(1);
                });

            child.wait();
            parent_executed.fetch_add(1);
        });

    parent.wait();

    CHECK(child_executed.load() == 1);
    CHECK(parent_executed.load() == 1);
}

TEST_CASE("task_scheduler: dispatch and wait from an external thread", "[task2][scheduler]")
{
    scheduler_scope scope(2);
    task_scheduler& scheduler = scope.get();

    std::atomic<std::int32_t> executed{0};
    std::atomic<bool> complete{false};

    std::thread external(
        [&]()
        {
            task_handle handle = scheduler.dispatch(
                [&]()
                {
                    executed.fetch_add(1);
                });

            handle.wait();
            complete.store(handle.is_complete());
        });

    external.join();

    CHECK(executed.load() == 1);
    CHECK(complete.load());
}

TEST_CASE("task_scheduler: handles are copyable and observe completion", "[task2][scheduler]")
{
    scheduler_scope scope(2);
    task_scheduler& scheduler = scope.get();

    std::atomic<std::int32_t> executed{0};
    task_handle first = scheduler.dispatch(
        [&]()
        {
            executed.fetch_add(1);
        });

    task_handle second = first; // both handles share the same block
    REQUIRE(first.is_valid());
    REQUIRE(second.is_valid());

    first.wait();
    CHECK(first.is_complete());
    CHECK(second.is_complete());

    first = task_handle{};
    CHECK_FALSE(first.is_valid());
    CHECK(second.is_valid());

    second.wait();
    second = task_handle{};
    CHECK_FALSE(second.is_valid());

    CHECK(executed.load() == 1);
}

TEST_CASE("task_scheduler: empty handles are inert", "[task2][scheduler]")
{
    // No scheduler needed for this one.
    task_handle empty;
    CHECK_FALSE(empty.is_valid());
    CHECK_FALSE(empty.is_complete());

    empty.wait(); // waiting on an empty handle must not crash

    task_handle copy = empty;
    CHECK_FALSE(copy.is_valid());

    copy = empty;
    CHECK_FALSE(copy.is_valid());

    task_handle moved = std::move(copy);
    CHECK_FALSE(moved.is_valid());
    CHECK_FALSE(copy.is_valid());
}

TEST_CASE("task_scheduler: repeated run and stop cycles", "[task2][scheduler]")
{
    constexpr std::int32_t cycle_count = 32;

    task_scheduler& scheduler = task_scheduler::instance();

    for (std::int32_t cycle = 0; cycle < cycle_count; ++cycle)
    {
        scheduler.run(2);

        std::atomic<std::int32_t> executed{0};
        task_handle handle = scheduler.dispatch(
            [&]()
            {
                executed.fetch_add(1);
            });

        handle.wait();
        scheduler.stop();

        REQUIRE(executed.load() == 1);
    }
}

TEST_CASE("task_scheduler: tasks occupy several workers at the same time", "[task2][scheduler]")
{
    constexpr std::int32_t task_count = 3; // one per compute worker

    scheduler_scope scope(4); // three compute workers plus one io worker
    task_scheduler& scheduler = scope.get();

    std::atomic<std::int32_t> started{0};
    std::atomic<bool> release{false};

    std::vector<task_handle> handles;
    handles.reserve(task_count);

    for (std::int32_t i = 0; i < task_count; ++i)
    {
        handles.push_back(scheduler.dispatch(
            [&]()
            {
                started.fetch_add(1);

                while (!release.load(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }
            }));
    }

    // Never fail before releasing the tasks, otherwise the scope destructor would join
    // threads that are still spinning.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (started.load() < task_count && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }

    const std::int32_t observed = started.load();
    release.store(true, std::memory_order_release);

    for (task_handle& handle : handles)
    {
        handle.wait();
    }

    CHECK(observed == task_count);
}
} // namespace violet::test
