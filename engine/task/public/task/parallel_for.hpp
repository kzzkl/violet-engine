#pragma once

#include "task/task_scheduler.hpp"
#include <algorithm>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>

namespace violet
{
template <typename F>
void parallel_for(std::size_t begin, std::size_t end, F&& fn, std::size_t batch_size = 0)
{
    if (begin >= end)
    {
        return;
    }

    auto& scheduler = task_scheduler::instance();

    const std::size_t count = end - begin;
    const std::size_t workers = std::max<std::size_t>(scheduler.get_worker_count(), 1);
    const std::size_t max_chunks = workers * 4;

    if (batch_size == 0)
    {
        batch_size = std::clamp<std::size_t>(count / max_chunks, 1, 4096);
    }

    std::size_t chunk_count = (count + batch_size - 1) / batch_size;
    if (chunk_count > max_chunks)
    {
        chunk_count = max_chunks;
        batch_size = (count + chunk_count - 1) / chunk_count;
    }

    auto run_chunk = [&fn, begin, end, batch_size](std::size_t chunk)
    {
        const std::size_t s = begin + (chunk * batch_size);
        const std::size_t e = std::min(s + batch_size, end);

        for (std::size_t index = s; index < e; ++index)
        {
            fn(index);
        }
    };

    if (chunk_count <= 1)
    {
        run_chunk(0);
        return;
    }

    task_event event(static_cast<std::uint32_t>(chunk_count - 1));

    for (std::size_t chunk = 1; chunk < chunk_count; ++chunk)
    {
        scheduler.dispatch(
            [&run_chunk, &event, chunk]
            {
                run_chunk(chunk);
                event.notify();
            });
    }

    run_chunk(0);
    event.wait();
}

template <typename T, std::size_t N, typename F>
void parallel_for(std::span<T, N> range, F&& fn, std::size_t batch_size = 0)
{
    parallel_for(
        std::size_t{0},
        range.size(),
        [range, &fn](std::size_t index)
        {
            fn(index, range[index]);
        },
        batch_size);
}

namespace detail
{
template <typename>
inline constexpr bool is_span_v = false;

template <typename T, std::size_t N>
inline constexpr bool is_span_v<std::span<T, N>> = true;

template <typename R>
concept non_span_contiguous_range =
    std::ranges::contiguous_range<R> && std::ranges::sized_range<R> &&
    !is_span_v<std::remove_cvref_t<R>>;
} // namespace detail

template <detail::non_span_contiguous_range R, typename F>
void parallel_for(R&& range, F&& fn, std::size_t batch_size = 0)
{
    parallel_for(std::span{range}, std::forward<F>(fn), batch_size);
}
} // namespace violet