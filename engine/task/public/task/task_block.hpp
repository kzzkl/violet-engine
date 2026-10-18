#pragma once

#include <atomic>
#include <functional>

namespace violet
{
enum task_affinity : std::uint8_t
{
    TASK_AFFINITY_ANY,
    TASK_AFFINITY_MAIN,
    TASK_AFFINITY_BLOCKING,
};

enum task_state : std::uint8_t
{
    TASK_STATE_WAITING,
    TASK_STATE_RUNNING,
    TASK_STATE_COMPLETE,
};

class task_scheduler;

struct task_block
{
    std::atomic<std::uint32_t> reference_count{1};
    std::atomic<task_state> state{TASK_STATE_WAITING};

    std::atomic<task_block*> continuation_head{nullptr};
    task_block* dispatch_next{nullptr};

    task_affinity affinity;

    std::function<void()> function;

    task_scheduler* scheduler{nullptr};
};
} // namespace violet