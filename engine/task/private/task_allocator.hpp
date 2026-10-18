#pragma once

#include "task/task_block.hpp"
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

        if (m_free_blocks.size() > 500)
        {
            for (std::size_t i = 500; i < m_free_blocks.size(); ++i)
            {
                delete m_free_blocks[i];
            }

            m_free_blocks.erase(m_free_blocks.begin(), m_free_blocks.begin() + 500);
        }
    }

private:
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

    task_block* allocate()
    {
        if (m_free_blocks.empty())
        {
            m_free_blocks.resize(16);

            std::size_t allocate_count = m_shared_allocator->allocate(m_free_blocks);

            for (std::size_t i = allocate_count; i < 16; ++i)
            {
                m_free_blocks[i] = new task_block();
            }
        }

        task_block* block = m_free_blocks.back();
        m_free_blocks.pop_back();
        return block;
    }

    void free(task_block* block)
    {
        m_free_blocks.push_back(block);

        if (m_free_blocks.size() > 100)
        {
            m_shared_allocator->free(std::span(m_free_blocks.begin() + 100, m_free_blocks.end()));
            m_free_blocks.resize(100);
        }
    }

private:
    std::vector<task_block*> m_free_blocks;

    task_allocator_shared* m_shared_allocator;
};
} // namespace violet