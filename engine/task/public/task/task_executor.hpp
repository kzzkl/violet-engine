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

    template <typename G, typename... Args>
    std::future<void> execute(G& graph, Args&&... args)
    {
        std::future<void> future = graph.reset();
        if (graph.get_task_count() > 1)
        {
            graph.set_argument(std::forward<Args>(args)...);
            execute_task(&graph.get_root());
            execute_main_thread_task(graph.get_task_count(TASK_TYPE_MAIN_THREAD));
        }

        return future;
    }

    template <typename G, typename... Args>
    void execute_sync(G& graph, Args&&... args)
    {
        std::future<void> future = graph.reset();
        if (graph.get_task_count() > 1)
        {
            graph.set_argument(std::forward<Args>(args)...);
            execute_task(&graph.get_root());
            execute_main_thread_task(graph.get_task_count(TASK_TYPE_MAIN_THREAD));
            future.get();
        }
    }

    void run(std::size_t thread_count = 0);
    void stop();

private:
    class thread_pool;

    void execute_task(task_base* task);
    void execute_main_thread_task(std::size_t task_count);

    std::array<std::unique_ptr<task_queue>, 2> m_queues;

    std::unique_ptr<thread_pool> m_thread_pool;

    std::atomic<bool> m_stop;
};
} // namespace violet