#pragma once

#include "task/task_block.hpp"
#include <array>
#include <mutex>
#include <span>

namespace violet
{
class task_allocator_shared
{
public:
    task_block* allocate()
    {
        std::scoped_lock lock(m_mutex);

        if (m_free_blocks.empty())
        {
            return new task_block();
        }

        task_block* block = m_free_blocks.back();
        m_free_blocks.pop_back();
        return block;
    }

    std::size_t allocate(std::span<task_block*> blocks)
    {
        std::scoped_lock lock(m_mutex);

        std::size_t count = std::min(blocks.size(), m_free_blocks.size());

        for (std::size_t i = 0; i < count; ++i)
        {
            blocks[i] = m_free_blocks.back();
            m_free_blocks.pop_back();
        }

        return count;
    }

    void free(task_block* block)
    {
        std::scoped_lock lock(m_mutex);

        block->continuation_head = nullptr;
        m_free_blocks.push_back(block);
    }

    void free(std::span<task_block*> blocks)
    {
        std::scoped_lock lock(m_mutex);

        m_free_blocks.insert(m_free_blocks.end(), blocks.begin(), blocks.end());

        if (m_free_blocks.size() > capacity)
        {
            for (std::size_t i = capacity; i < m_free_blocks.size(); ++i)
            {
                delete m_free_blocks[i];
            }

            m_free_blocks.resize(capacity);
        }
    }

private:
    static constexpr std::size_t capacity = 4096;

    std::vector<task_block*> m_free_blocks;

    std::mutex m_mutex;
};

class task_allocator_local
{
public:
    task_allocator_local(task_allocator_shared* shared_allocator)
        : m_shared_allocator(shared_allocator)
    {
    }

    ~task_allocator_local()
    {
        if (m_count > 0)
        {
            m_shared_allocator->free(
                std::span(
                    m_blocks.begin(),
                    m_blocks.begin() + static_cast<std::ptrdiff_t>(m_count)));
        }
    }

    task_block* allocate()
    {
        if (m_count == 0)
        {
            std::size_t allocate_count = m_shared_allocator->allocate(
                std::span(m_blocks.begin(), m_blocks.begin() + batch_size));

            for (std::size_t i = allocate_count; i < batch_size; ++i)
            {
                m_blocks[i] = new task_block();
            }

            m_count = batch_size;
        }

        return m_blocks[--m_count];
    }

    void free(task_block* block)
    {
        block->continuation_head = nullptr;

        std::size_t slot = m_count++;

        m_blocks[slot] = block;

        if (m_count > high_water)
        {
            m_shared_allocator->free(
                std::span(
                    m_blocks.begin() + low_water,
                    m_blocks.begin() + static_cast<std::ptrdiff_t>(m_count)));
            m_count = low_water;
        }
    }

private:
    static constexpr std::size_t capacity = 256;
    static constexpr std::size_t high_water = 200;
    static constexpr std::size_t low_water = 100;
    static constexpr std::size_t batch_size = 16;

    std::array<task_block*, capacity> m_blocks;
    std::size_t m_count{0};

    task_allocator_shared* m_shared_allocator;
};
} // namespace violet