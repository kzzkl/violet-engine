#pragma once

#include "task/task_graph.hpp"
#include "task/task_queue.hpp"

namespace violet
{
class task_executor
{
public:
    using task_queue = task_queue_thread_safe<task_wrapper>;

public:
    task_executor();
    ~task_executor();

    std::future<void> execute(task_graph& task_graph)
    {
        std::future<void> future = task_graph.reset();

        auto& root_tasks = task_graph.get_root_tasks();
        if (!root_tasks.empty())
        {
            for (task_wrapper* task : root_tasks)
            {
                execute_task(task);
            }
            execute_main_thread_task(task_graph.get_main_thread_task_count());
        }

        return future;
    }

    void execute_sync(task_graph& task_graph)
    {
        std::future<void> future = task_graph.reset();

        auto& root_tasks = task_graph.get_root_tasks();
        if (!root_tasks.empty())
        {
            for (task_wrapper* task : root_tasks)
            {
                execute_task(task);
            }
            execute_main_thread_task(task_graph.get_main_thread_task_count());
            future.get();
        }
    }

    void run(std::size_t thread_count = 0);
    void stop();

private:
    class thread_pool;

    void execute_task(task_wrapper* task);
    void execute_main_thread_task(std::size_t task_count);

    void on_task_completed(task_wrapper* task);

    task_queue m_main_thread_queue;
    task_queue m_worker_thread_queue;

    std::unique_ptr<thread_pool> m_thread_pool;

    std::atomic<bool> m_stop;
};
} // namespace violet