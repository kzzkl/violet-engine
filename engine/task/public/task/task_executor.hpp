#pragma once

#include "task/task.hpp"

namespace violet
{
class task_queue;
class task_executor
{
public:
    task_executor();
    ~task_executor();

    std::future<void> execute(taskflow& taskflow)
    {
        std::future<void> future = taskflow.reset();
        if (taskflow.get_task_count() > 1)
        {
            execute_task(&taskflow.get_root());
            execute_main_thread_task(taskflow.get_main_thread_task_count());
        }

        return future;
    }

    void execute_sync(taskflow& taskflow)
    {
        std::future<void> future = taskflow.reset();
        if (taskflow.get_task_count() > 1)
        {
            execute_task(&taskflow.get_root());
            execute_main_thread_task(taskflow.get_main_thread_task_count());
            future.get();
        }
    }

    void run(std::size_t thread_count = 0);
    void stop();

private:
    class thread_pool;

    void execute_task(task* task);
    void execute_main_thread_task(std::size_t task_count);

    std::unique_ptr<task_queue> m_normal_queue;
    std::unique_ptr<task_queue> m_main_thread_queue;

    std::unique_ptr<thread_pool> m_thread_pool;

    std::atomic<bool> m_stop;
};
} // namespace violet