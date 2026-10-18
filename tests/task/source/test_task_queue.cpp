#include "task_queue.hpp"
#include <atomic>
#include <catch2/catch_all.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

namespace violet::test
{
namespace
{
/**
 * @brief A fixed set of task blocks plus a per-block consumption counter.
 *
 * The counter is what makes "every block is executed exactly once" checkable after the
 * fact: a lost block stays at 0, a duplicated block reaches 2. Assertions are made only
 * after the worker threads are joined, since Catch2 is not thread safe.
 */
class block_pool
{
public:
    explicit block_pool(std::size_t count)
        : m_blocks(std::make_unique<task_block[]>(count)),
          m_seen(std::make_unique<std::atomic<std::int32_t>[]>(count)),
          m_count(count)
    {
        for (std::size_t i = 0; i < m_count; ++i)
        {
            m_seen[i].store(0, std::memory_order_relaxed);
        }
    }

    task_block* get(std::size_t index) noexcept
    {
        return &m_blocks[index];
    }

    void mark(const task_block* block) noexcept
    {
        const std::ptrdiff_t index = block - m_blocks.get();
        m_seen[index].fetch_add(1, std::memory_order_relaxed);
    }

    std::int32_t total_consumed() const noexcept
    {
        std::int32_t total = 0;
        for (std::size_t i = 0; i < m_count; ++i)
        {
            total += m_seen[i].load(std::memory_order_relaxed);
        }
        return total;
    }

    bool all_consumed_once() const noexcept
    {
        for (std::size_t i = 0; i < m_count; ++i)
        {
            if (m_seen[i].load(std::memory_order_relaxed) != 1)
            {
                return false;
            }
        }
        return true;
    }

private:
    std::unique_ptr<task_block[]> m_blocks;
    std::unique_ptr<std::atomic<std::int32_t>[]> m_seen;
    std::size_t m_count;
};
} // namespace

// ---------------------------------------------------------------------------------------------
// task_work_queue: per-worker local queue (Chase-Lev). Owner pops LIFO, thieves steal FIFO.
// ---------------------------------------------------------------------------------------------

TEST_CASE("task_work_queue is empty after construction", "[task2][work_queue]")
{
    task_work_queue queue(16);

    CHECK(queue.try_pop() == nullptr);
    CHECK(queue.try_steal() == nullptr);
}

TEST_CASE("task_work_queue pops LIFO on the owner end", "[task2][work_queue]")
{
    block_pool pool(4);
    task_work_queue queue(16);

    REQUIRE(queue.push(pool.get(0)));
    REQUIRE(queue.push(pool.get(1)));
    REQUIRE(queue.push(pool.get(2)));

    CHECK(queue.try_pop() == pool.get(2));
    CHECK(queue.try_pop() == pool.get(1));
    CHECK(queue.try_pop() == pool.get(0));
    CHECK(queue.try_pop() == nullptr);
}

TEST_CASE("task_work_queue steals FIFO on the thief end", "[task2][work_queue]")
{
    block_pool pool(4);
    task_work_queue queue(16);

    REQUIRE(queue.push(pool.get(0)));
    REQUIRE(queue.push(pool.get(1)));
    REQUIRE(queue.push(pool.get(2)));

    CHECK(queue.try_steal() == pool.get(0));
    CHECK(queue.try_steal() == pool.get(1));
    CHECK(queue.try_steal() == pool.get(2));
    CHECK(queue.try_steal() == nullptr);

    CHECK(queue.try_pop() == nullptr);
}

TEST_CASE("task_work_queue interleaves owner pops and steals", "[task2][work_queue]")
{
    block_pool pool(4);
    task_work_queue queue(16);

    REQUIRE(queue.push(pool.get(0)));
    REQUIRE(queue.push(pool.get(1)));
    REQUIRE(queue.push(pool.get(2)));
    REQUIRE(queue.push(pool.get(3)));

    CHECK(queue.try_steal() == pool.get(0)); // one element from each end
    CHECK(queue.try_pop() == pool.get(3));

    CHECK(queue.try_steal() == pool.get(1));
    CHECK(queue.try_pop() == pool.get(2));

    CHECK(queue.try_pop() == nullptr);
    CHECK(queue.try_steal() == nullptr);
}

TEST_CASE(
    "task_work_queue rejects pushes beyond capacity and wraps correctly",
    "[task2][work_queue]")
{
    constexpr std::size_t capacity = 8;
    constexpr std::size_t half = capacity / 2;

    block_pool pool(capacity * 2);
    task_work_queue queue(capacity);

    for (std::size_t i = 0; i < capacity; ++i)
    {
        REQUIRE(queue.push(pool.get(i)));
    }

    // Full: an extra push must be rejected rather than overwriting a live element.
    CHECK_FALSE(queue.push(pool.get(capacity)));

    // Pop half of it (LIFO: 7,6,5,4), then push the same amount back to force a wrap-around.
    for (std::size_t i = 0; i < half; ++i)
    {
        REQUIRE(queue.try_pop() == pool.get(capacity - 1 - i));
    }
    for (std::size_t i = 0; i < half; ++i)
    {
        REQUIRE(queue.push(pool.get(capacity + i)));
    }

    // The remaining old elements are still 0..3, and a thief still steals oldest first.
    for (std::size_t i = 0; i < half; ++i)
    {
        CHECK(queue.try_steal() == pool.get(i));
    }

    // The newly pushed 8..11 come back in LIFO order.
    for (std::size_t i = 0; i < half; ++i)
    {
        CHECK(queue.try_pop() == pool.get(capacity + half - 1 - i));
    }

    CHECK(queue.try_pop() == nullptr);
}

TEST_CASE(
    "task_work_queue concurrent steal consumes every block exactly once",
    "[task2][work_queue]")
{
    constexpr std::size_t block_count = 4096;

    block_pool pool(block_count);
    task_work_queue queue(block_count);

    for (std::size_t i = 0; i < block_count; ++i)
    {
        REQUIRE(queue.push(pool.get(i)));
    }

    std::atomic<std::uint32_t> claimed{0};
    std::atomic<bool> done{false};

    auto claim = [&](task_block* block)
    {
        if (block == nullptr)
        {
            return;
        }

        pool.mark(block);

        if (claimed.fetch_add(1, std::memory_order_acq_rel) + 1 == block_count)
        {
            done.store(true, std::memory_order_release);
        }
    };

    std::thread thief_1(
        [&]()
        {
            while (!done.load(std::memory_order_acquire))
            {
                claim(queue.try_steal());
            }
        });

    std::thread thief_2(
        [&]()
        {
            while (!done.load(std::memory_order_acquire))
            {
                claim(queue.try_steal());
            }
        });

    std::thread owner(
        [&]()
        {
            while (!done.load(std::memory_order_acquire))
            {
                claim(queue.try_pop());
            }
        });

    owner.join();
    thief_1.join();
    thief_2.join();

    CHECK(claimed.load() == block_count);
    CHECK(pool.total_consumed() == static_cast<std::int32_t>(block_count));
    CHECK(pool.all_consumed_once());
}

TEST_CASE("task_work_queue single-element race has exactly one winner", "[task2][work_queue]")
{
    // With a single element left, the owner's pop and a thief's steal race for the same
    // slot: exactly one side must win, never both and never neither.
    constexpr std::size_t round_count = 200;

    block_pool pool(2);
    task_work_queue queue(4);

    for (std::size_t round = 0; round < round_count; ++round)
    {
        REQUIRE(queue.push(pool.get(0)));

        std::atomic<std::uint32_t> winners{0};
        std::atomic<bool> start{false};

        auto attempt = [&](bool steal)
        {
            while (!start.load(std::memory_order_acquire))
            {
            }

            task_block* block = steal ? queue.try_steal() : queue.try_pop();
            if (block != nullptr)
            {
                winners.fetch_add(1, std::memory_order_relaxed);
            }
        };

        std::thread owner(attempt, false);
        std::thread thief_1(attempt, true);
        std::thread thief_2(attempt, true);

        start.store(true, std::memory_order_release);

        owner.join();
        thief_1.join();
        thief_2.join();

        CHECK(winners.load() == 1);

        // Safety net: the queue should already be empty here.
        while (queue.try_pop() != nullptr)
        {
        }
        while (queue.try_steal() != nullptr)
        {
        }
    }
}

// ---------------------------------------------------------------------------------------------
// task_block_queue: global / main / io (bounded MPMC). FIFO on both ends.
// ---------------------------------------------------------------------------------------------

TEST_CASE("task_block_queue is empty after construction", "[task2][block_queue]")
{
    task_block_queue queue(16);

    CHECK(queue.try_pop() == nullptr);
}

TEST_CASE("task_block_queue pops FIFO", "[task2][block_queue]")
{
    block_pool pool(4);
    task_block_queue queue(16);

    REQUIRE(queue.push(pool.get(0)));
    REQUIRE(queue.push(pool.get(1)));
    REQUIRE(queue.push(pool.get(2)));

    CHECK(queue.try_pop() == pool.get(0));
    CHECK(queue.try_pop() == pool.get(1));
    CHECK(queue.try_pop() == pool.get(2));
    CHECK(queue.try_pop() == nullptr);
}

TEST_CASE(
    "task_block_queue rejects pushes beyond capacity and wraps correctly",
    "[task2][block_queue]")
{
    constexpr std::size_t capacity = 8;
    constexpr std::size_t half = capacity / 2;

    block_pool pool(capacity * 2);
    task_block_queue queue(capacity);

    for (std::size_t i = 0; i < capacity; ++i)
    {
        REQUIRE(queue.push(pool.get(i)));
    }

    CHECK_FALSE(queue.push(pool.get(capacity)));

    // Pop the first half, then push again to force the sequence to wrap to the array head.
    for (std::size_t i = 0; i < half; ++i)
    {
        REQUIRE(queue.try_pop() == pool.get(i));
    }
    for (std::size_t i = 0; i < half; ++i)
    {
        REQUIRE(queue.push(pool.get(capacity + i)));
    }

    for (std::size_t i = 0; i < half; ++i)
    {
        CHECK(queue.try_pop() == pool.get(half + i));
    }
    for (std::size_t i = 0; i < half; ++i)
    {
        CHECK(queue.try_pop() == pool.get(capacity + i));
    }

    CHECK(queue.try_pop() == nullptr);
}

TEST_CASE(
    "task_block_queue concurrent producers and consumers lose nothing",
    "[task2][block_queue]")
{
    constexpr std::size_t block_count = 20000;
    constexpr std::size_t producer_count = 4;
    constexpr std::size_t consumer_count = 4;

    block_pool pool(block_count);
    task_block_queue queue(1024); // far below block_count, so the queue fills up and wraps

    std::atomic<std::size_t> next{0};
    std::atomic<std::uint32_t> produced{0};
    std::atomic<std::uint32_t> consumed{0};

    auto produce = [&]()
    {
        while (true)
        {
            const std::size_t index = next.fetch_add(1, std::memory_order_relaxed);
            if (index >= block_count)
            {
                return;
            }

            while (!queue.push(pool.get(index)))
            {
                std::this_thread::yield(); // full: wait for a consumer to free a slot
            }

            produced.fetch_add(1, std::memory_order_release);
        }
    };

    auto consume = [&]()
    {
        while (consumed.load(std::memory_order_acquire) < block_count)
        {
            task_block* block = queue.try_pop();
            if (block == nullptr)
            {
                std::this_thread::yield();
                continue;
            }

            pool.mark(block);
            consumed.fetch_add(1, std::memory_order_release);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(producer_count + consumer_count);

    for (std::size_t i = 0; i < producer_count; ++i)
    {
        threads.emplace_back(produce);
    }
    for (std::size_t i = 0; i < consumer_count; ++i)
    {
        threads.emplace_back(consume);
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    CHECK(produced.load() == block_count);
    CHECK(consumed.load() == block_count);
    CHECK(pool.total_consumed() == static_cast<std::int32_t>(block_count));
    CHECK(pool.all_consumed_once());
}
} // namespace violet::test
