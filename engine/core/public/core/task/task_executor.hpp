#pragma once

#include "task.hpp"

namespace violet
{
class task_queue;
class task_executor
{
public:
    task_executor();
    ~task_executor();

    template <typename... Args>
    std::future<void> execute(task_graph<Args...>& graph, const Args&... args)
    {
        std::future<void> future = graph.prepare(args...);
        if (graph.get_task_count(TASK_OPTION_NONE) > 1)
        {
            execute_task(graph.get_root());

            std::size_t main_thread_task_count = graph.get_task_count(TASK_OPTION_MAIN_THREAD);
            execute_main_thread_task(main_thread_task_count);
        }

        return future;
    }

    template <typename... Args>
    void execute_sync(task_graph<Args...>& graph, const Args&... args)
    {
        if (graph.get_task_count(TASK_OPTION_NONE) > 1)
        {
            std::future<void> future = execute(graph, args...);
            future.get();
        }
    }

    void run(std::size_t thread_count = 0);
    void stop();

private:
    class thread_pool;

    void execute_task(task* task);
    void execute_main_thread_task(std::size_t task_count);

    std::unique_ptr<task_queue> m_queue;
    std::unique_ptr<task_queue> m_main_thread_queue;
    std::unique_ptr<thread_pool> m_thread_pool;

    std::atomic<bool> m_stop;
};
} // namespace violet