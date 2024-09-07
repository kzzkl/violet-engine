#include "task/task_graph.hpp"
#include <stack>

namespace violet
{
task_graph::task_graph() noexcept
    : m_dirty(false),
      m_incomplete_count(0)
{
}

std::future<void> task_graph::reset() noexcept
{
    if (m_dirty)
    {
        compile();
        m_dirty = false;
    }

    m_incomplete_count = m_main_thread_task_count + m_worker_thread_task_count;
    m_promise = std::promise<void>();

    return m_promise.get_future();
}

std::size_t task_graph::get_task_count() const noexcept
{
    return m_tasks.size();
}

void task_graph::notify_task_complete()
{
    m_incomplete_count.fetch_sub(1);

    std::uint32_t expected = 0;
    if (m_incomplete_count.compare_exchange_strong(
            expected, static_cast<std::uint32_t>(m_tasks.size())))
    {
        m_promise.set_value();
    }
}

void task_graph::compile()
{
    m_roots.clear();
    m_main_thread_task_count = 0;
    m_worker_thread_task_count = 0;

    for (auto& task : m_tasks)
    {
        if (!task->is_empty())
        {
            if (task->get_options() & TASK_OPTION_MAIN_THREAD)
            {
                ++m_main_thread_task_count;
            }
            else
            {
                ++m_worker_thread_task_count;
            }
        }

        task->dependents.clear();
        task->successors.clear();

        if (task->get_dependents().empty())
        {
            m_roots.push_back(task.get());
        }
    }

    transitive_reduction();
}

void task_graph::transitive_reduction()
{
    // Topological sort.
    std::unordered_map<task*, std::size_t> in_edge;
    for (auto& task : m_tasks)
    {
        in_edge[task.get()] = task->get_dependents().size();
    }

    std::stack<task*> stack;
    for (task_wrapper* root : m_roots)
    {
        stack.push(root);
    }

    std::vector<task_wrapper*> sorted_tasks;
    while (!stack.empty())
    {
        task* node = stack.top();
        stack.pop();

        for (task* successor : node->get_successors())
        {
            if ((--in_edge[successor]) == 0)
            {
                stack.push(successor);
            }
        }

        sorted_tasks.push_back(static_cast<task_wrapper*>(node));
    }

    // Transitive reduction.
    std::unordered_map<task*, std::uint8_t> task_flags;
    for (int i = 1; i < sorted_tasks.size(); ++i)
    {
        for (int j = 0; j < i; ++j)
        {
            task_flags[sorted_tasks[j]] = 0;
        }

        for (int j = i - 1; j >= 0; --j)
        {
            task_wrapper* prev_task = sorted_tasks[j];
            task_wrapper* curr_task = sorted_tasks[i];

            if (task_flags[prev_task] == 0)
            {
                auto prev_task_successors = prev_task->get_successors();
                auto iter =
                    std::find(prev_task_successors.begin(), prev_task_successors.end(), curr_task);
                if (iter != prev_task_successors.end())
                {
                    curr_task->dependents.push_back(prev_task);
                    prev_task->successors.push_back(curr_task);
                    task_flags[prev_task] = 1;
                }
            }

            if (task_flags[prev_task] != 0)
            {
                for (task* dependent : prev_task->get_dependents())
                {
                    task_flags[dependent] = 1;
                }
            }
        }
    }
}
} // namespace violet