#pragma once

#include "core/task/task.hpp"
#include "task/lock_free_queue.hpp"
#include "task/thread_safe_queue.hpp"

namespace violet
{
class task_queue
{
public:
    // using queue_type = lock_free_queue<task*>;
    using queue_type = thread_safe_queue<task*>;

public:
    task_queue();

    task* pop();
    void push(task* task);

    void start();
    void stop();

private:
    queue_type m_queue;
    std::atomic<bool> m_exit;
};
} // namespace violet