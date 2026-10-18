#include "task_queue.hpp"
#include <cassert>

namespace violet
{
task_work_queue::task_work_queue(std::size_t capacity)
    : m_capacity(capacity),
      m_mask(capacity - 1)
{
    assert((capacity & (capacity - 1)) == 0 && capacity != 0);

    m_blocks.resize(capacity);
}

bool task_work_queue::push(task_block* block) noexcept
{
    const std::size_t bottom = m_bottom.load(std::memory_order_relaxed);
    const std::size_t top = m_top.load(std::memory_order_acquire);

    if (bottom - top >= m_capacity)
    {
        return false;
    }

    m_blocks[bottom & m_mask] = block;

    std::atomic_thread_fence(std::memory_order_release);
    m_bottom.store(bottom + 1, std::memory_order_relaxed);

    return true;
}

task_block* task_work_queue::try_pop() noexcept
{
    std::size_t bottom = m_bottom.load(std::memory_order_relaxed);
    if (bottom == 0)
    {
        return nullptr;
    }

    --bottom;
    m_bottom.store(bottom, std::memory_order_relaxed);

    std::atomic_thread_fence(std::memory_order_seq_cst);
    std::size_t top = m_top.load(std::memory_order_relaxed);

    if (top > bottom)
    {
        m_bottom.store(bottom + 1, std::memory_order_relaxed);
        return nullptr;
    }

    task_block* block = m_blocks[bottom & m_mask];

    if (top == bottom)
    {
        if (!m_top.compare_exchange_strong(
                top,
                top + 1,
                std::memory_order_seq_cst,
                std::memory_order_relaxed))
        {
            block = nullptr;
        }

        m_bottom.store(bottom + 1, std::memory_order_relaxed);
    }

    return block;
}

task_block* task_work_queue::try_steal() noexcept
{
    std::size_t top = m_top.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    const std::size_t bottom = m_bottom.load(std::memory_order_acquire);

    if (top >= bottom)
    {
        return nullptr;
    }

    task_block* block = m_blocks[top & m_mask];

    if (!m_top.compare_exchange_strong(
            top,
            top + 1,
            std::memory_order_seq_cst,
            std::memory_order_relaxed))
    {
        return nullptr;
    }

    return block;
}

task_block_queue::task_block_queue(std::size_t capacity)
    : m_slots(capacity),
      m_mask(capacity - 1)
{
    assert((capacity & m_mask) == 0 && capacity != 0);

    for (std::size_t i = 0; i < capacity; ++i)
    {
        m_slots[i].sequence.store(i, std::memory_order_relaxed);
    }
}

bool task_block_queue::push(task_block* block) noexcept
{
    std::size_t pos = m_tail.load(std::memory_order_relaxed);

    while (true)
    {
        auto& slot = m_slots[pos & m_mask];

        const std::size_t seq = slot.sequence.load(std::memory_order_acquire);
        const std::ptrdiff_t diff =
            static_cast<std::ptrdiff_t>(seq) - static_cast<std::ptrdiff_t>(pos);

        if (diff == 0)
        {
            if (m_tail.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
            {
                slot.block = block;
                slot.sequence.store(pos + 1, std::memory_order_release);
                return true;
            }
        }
        else if (diff < 0)
        {
            return false;
        }
        else
        {
            pos = m_tail.load(std::memory_order_relaxed);
        }
    }
}

task_block* task_block_queue::try_pop() noexcept
{
    std::size_t pos = m_head.load(std::memory_order_relaxed);

    while (true)
    {
        auto& slot = m_slots[pos & m_mask];

        const std::size_t seq = slot.sequence.load(std::memory_order_acquire);
        const std::ptrdiff_t diff =
            static_cast<std::ptrdiff_t>(seq) - static_cast<std::ptrdiff_t>(pos + 1);

        if (diff == 0)
        {
            if (m_head.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
            {
                task_block* block = slot.block;
                slot.sequence.store(pos + m_mask + 1, std::memory_order_release);
                return block;
            }
        }
        else if (diff < 0)
        {
            return nullptr;
        }
        else
        {
            pos = m_head.load(std::memory_order_relaxed);
        }
    }
}
} // namespace violet