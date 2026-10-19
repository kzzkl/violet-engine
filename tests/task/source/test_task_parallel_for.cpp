#include "task/parallel_for.hpp"
#include "test_task_common.hpp"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace violet::test
{
namespace
{
/**
 * @brief Counts how often each index was passed to the parallel_for body.
 *
 * A lost index stays at 0, a duplicated one reaches 2, so a single sweep after the call
 * tells both failure modes apart.
 */
class visit_counter
{
public:
    explicit visit_counter(std::size_t count)
        : m_visits(std::make_unique<std::atomic<std::int32_t>[]>(count)),
          m_count(count)
    {
        for (std::size_t i = 0; i < m_count; ++i)
        {
            m_visits[i].store(0);
        }
    }

    std::int32_t visit_count(std::size_t index) const
    {
        return m_visits[index].load();
    }

    std::int32_t total_visits() const
    {
        std::int32_t total = 0;

        for (std::size_t i = 0; i < m_count; ++i)
        {
            total += m_visits[i].load();
        }

        return total;
    }

    bool all_visited_once() const
    {
        for (std::size_t i = 0; i < m_count; ++i)
        {
            if (m_visits[i].load() != 1)
            {
                return false;
            }
        }

        return true;
    }

    void visit(std::size_t index)
    {
        m_visits[index].fetch_add(1);
    }

private:
    std::unique_ptr<std::atomic<std::int32_t>[]> m_visits;
    std::size_t m_count;
};
} // namespace

TEST_CASE("parallel_for covers every index exactly once", "[task][parallel_for]")
{
    constexpr std::size_t count = 10000;

    scheduler_scope scope(4);

    visit_counter visits(count);

    parallel_for(
        std::size_t{0},
        count,
        [&visits](std::size_t index)
        {
            visits.visit(index);
        });

    CHECK(visits.total_visits() == static_cast<std::int32_t>(count));
    CHECK(visits.all_visited_once());
}

TEST_CASE("parallel_for respects the half-open range", "[task][parallel_for]")
{
    constexpr std::size_t begin = 100;
    constexpr std::size_t end = 1000;
    constexpr std::size_t limit = 1100;

    scheduler_scope scope(4);

    visit_counter visits(limit);

    parallel_for(
        begin,
        end,
        [&visits](std::size_t index)
        {
            visits.visit(index);
        });

    std::int32_t before = 0;
    std::int32_t inside = 0;
    std::int32_t after = 0;

    for (std::size_t i = 0; i < limit; ++i)
    {
        const std::int32_t count = visits.visit_count(i);

        if (i < begin)
        {
            before += count;
        }
        else if (i < end)
        {
            inside += (count == 1) ? 1 : 0;
        }
        else
        {
            after += count;
        }
    }

    CHECK(before == 0);
    CHECK(inside == static_cast<std::int32_t>(end - begin));
    CHECK(after == 0);
}

TEST_CASE("parallel_for with an empty range returns immediately", "[task][parallel_for]")
{
    scheduler_scope scope(4);

    std::atomic<std::int32_t> calls{0};

    parallel_for(
        std::size_t{0},
        std::size_t{0},
        [&calls](std::size_t)
        {
            calls.fetch_add(1);
        });

    parallel_for(
        std::size_t{5},
        std::size_t{5},
        [&calls](std::size_t)
        {
            calls.fetch_add(1);
        });

    CHECK(calls.load() == 0);
}

TEST_CASE("parallel_for with begin past end returns immediately", "[task][parallel_for]")
{
    scheduler_scope scope(4);

    std::atomic<std::int32_t> calls{0};

    parallel_for(
        std::size_t{10},
        std::size_t{5},
        [&calls](std::size_t)
        {
            calls.fetch_add(1);
        });

    CHECK(calls.load() == 0);
}

TEST_CASE("parallel_for honours an explicit batch size", "[task][parallel_for]")
{
    constexpr std::size_t count = 100;
    constexpr std::size_t batch_size = 10;

    scheduler_scope scope(4);

    visit_counter visits(count);

    parallel_for(
        std::size_t{0},
        count,
        [&visits](std::size_t index)
        {
            visits.visit(index);
        },
        batch_size);

    CHECK(visits.total_visits() == static_cast<std::int32_t>(count));
    CHECK(visits.all_visited_once());
}

TEST_CASE("parallel_for handles a range larger than the chunk cap", "[task][parallel_for]")
{
    // Wider than the automatic batch size and the queue capacities, so the chunk
    // trimming path is exercised.
    constexpr std::size_t count = 1000000;

    scheduler_scope scope(4);

    std::atomic<std::uint64_t> sum{0};

    parallel_for(
        std::size_t{0},
        count,
        [&sum](std::size_t index)
        {
            sum.fetch_add(index, std::memory_order_relaxed);
        });

    CHECK(sum.load() == static_cast<std::uint64_t>(count - 1) * count / 2);
}

TEST_CASE("parallel_for called from a worker task", "[task][parallel_for]")
{
    constexpr std::size_t count = 1000;

    scheduler_scope scope(4);

    std::atomic<std::uint64_t> sum{0};

    task_handle handle = scope.get().dispatch(
        [&sum]()
        {
            // Runs on a worker: the chunks land in that worker's local queue, so the
            // waiting worker has to execute them itself instead of blocking.
            parallel_for(
                std::size_t{0},
                count,
                [&sum](std::size_t index)
                {
                    sum.fetch_add(index, std::memory_order_relaxed);
                });
        });

    handle.wait();

    CHECK(sum.load() == static_cast<std::uint64_t>(count - 1) * count / 2);
}

TEST_CASE("parallel_for with a single worker", "[task][parallel_for]")
{
    constexpr std::size_t count = 1000;

    scheduler_scope scope(1);

    visit_counter visits(count);

    parallel_for(
        std::size_t{0},
        count,
        [&visits](std::size_t index)
        {
            visits.visit(index);
        });

    CHECK(visits.total_visits() == static_cast<std::int32_t>(count));
    CHECK(visits.all_visited_once());
}

TEST_CASE("parallel_for over a span passes index and element", "[task][parallel_for]")
{
    constexpr std::size_t count = 1000;

    scheduler_scope scope(4);

    std::vector<std::int32_t> values(count, 0);

    // A non-default batch size also checks that it is forwarded by the overload.
    parallel_for(
        std::span(values),
        [](std::size_t index, std::int32_t& value)
        {
            value = static_cast<std::int32_t>(index) + 1;
        },
        std::size_t{64});

    bool correct = true;

    for (std::size_t i = 0; i < count; ++i)
    {
        if (values[i] != static_cast<std::int32_t>(i) + 1)
        {
            correct = false;
        }
    }

    CHECK(correct);
}

TEST_CASE("parallel_for over a const span yields const elements", "[task][parallel_for]")
{
    constexpr std::size_t count = 512;
    constexpr std::int32_t value_per_element = 7;

    scheduler_scope scope(4);

    const std::vector<std::int32_t> values(count, value_per_element);
    std::atomic<std::int32_t> sum{0};

    parallel_for(
        std::span(values),
        [&sum](std::size_t, const std::int32_t& value)
        {
            sum.fetch_add(value, std::memory_order_relaxed);
        });

    CHECK(sum.load() == static_cast<std::int32_t>(count) * value_per_element);
}

TEST_CASE("parallel_for over a container passes index and element", "[task][parallel_for]")
{
    constexpr std::size_t count = 1000;

    scheduler_scope scope(4);

    std::vector<std::int32_t> values(count, 0);

    parallel_for(
        values,
        [](std::size_t index, std::int32_t& value)
        {
            value = static_cast<std::int32_t>(index) * 2;
        });

    bool correct = true;

    for (std::size_t i = 0; i < count; ++i)
    {
        if (values[i] != static_cast<std::int32_t>(i) * 2)
        {
            correct = false;
        }
    }

    CHECK(correct);

    // A std::array exercises the static-extent span path.
    std::array<std::int32_t, 8> fixed{};

    parallel_for(
        fixed,
        [](std::size_t index, std::int32_t& value)
        {
            value = static_cast<std::int32_t>(index) + 1;
        });

    bool fixed_correct = true;

    for (std::size_t i = 0; i < fixed.size(); ++i)
    {
        if (fixed[i] != static_cast<std::int32_t>(i) + 1)
        {
            fixed_correct = false;
        }
    }

    CHECK(fixed_correct);
}

TEST_CASE("parallel_for with a single chunk runs inline", "[parallel_for-single-chunk]")
{
    scheduler_scope scope(4);

    std::atomic<std::int32_t> calls{0};

    // count == 1 with the automatic batch size.
    parallel_for(
        std::size_t{0},
        std::size_t{1},
        [&calls](std::size_t)
        {
            calls.fetch_add(1);
        });

    // A batch size larger than the range.
    parallel_for(
        std::size_t{0},
        std::size_t{10},
        [&calls](std::size_t)
        {
            calls.fetch_add(1);
        },
        std::size_t{100});

    CHECK(calls.load() == 11);
}
} // namespace violet::test
