#pragma once

#include "task/task_block.hpp"
#include <cstddef>
#include <vector>

namespace violet
{
class task_work_queue
{
public:
    task_work_queue(std::size_t capacity);

    bool push(task_block* block) noexcept;

    task_block* try_pop() noexcept;

    task_block* try_steal() noexcept;

    std::size_t get_size_approx() const noexcept;

private:
    std::size_t m_capacity;
    std::size_t m_mask;

    alignas(64) std::atomic<std::size_t> m_top{0};
    alignas(64) std::atomic<std::size_t> m_bottom{0};

    std::vector<task_block*> m_blocks;
};

class task_block_queue
{
public:
    task_block_queue(std::size_t capacity);

    bool push(task_block* block) noexcept;

    task_block* try_pop() noexcept;

private:
    struct slot
    {
        std::atomic<std::size_t> sequence{0};
        task_block* block{nullptr};
    };

    std::vector<slot> m_slots;
    std::size_t m_mask;

    alignas(64) std::atomic<std::size_t> m_head{0};
    alignas(64) std::atomic<std::size_t> m_tail{0};
};
} // namespace violet