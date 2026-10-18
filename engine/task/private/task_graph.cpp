#include "task/task_graph.hpp"
#include "task_graph/task_graph_printer.hpp"
#include <algorithm>
#include <cassert>
#include <stack>

namespace violet
{
task_graph::task_graph() noexcept = default;

task_handle task_graph::execute(task_scheduler& scheduler)
{
    assert(m_executing == false);

    if (m_dirty)
    {
        compile();
        m_dirty = false;
    }

    m_executing = true;

    std::uint32_t leaf_count = 0;

    for (auto& task : m_tasks)
    {
        task->event.reset();
        task->event.add(static_cast<std::uint32_t>(task->dependencies.size()));

        leaf_count += task->successors.empty() ? 1 : 0;
    }

    m_done.reset();
    m_done.add(leaf_count);

    for (auto& task : m_tasks)
    {
        task->handle = task->event.then(
            [n = task.get(), this]()
            {
                n->execute();

                for (task_wrapper* successor : n->successors)
                {
                    successor->event.notify();
                }

                if (n->successors.empty())
                {
                    m_done.notify();
                }
            },
            task->get_affinity());
    }

    return m_done.then(
        [&]()
        {
            m_executing = false;
        });
}

void task_graph::print()
{
    if (m_dirty)
    {
        compile();
        m_dirty = false;
    }

    task_graph_printer::print(*this);
}

void task_graph::compile()
{
    assert(m_executing == false);

    m_roots.clear();

    for (auto& task : m_tasks)
    {
        task->dependencies.clear();
        task->successors.clear();

        if (task->get_dependencies().empty())
        {
            m_roots.push_back(task.get());
        }
    }

    transitive_reduction();
}

void task_graph::transitive_reduction()
{
    // Topological sort.
    std::unordered_map<task_node*, std::size_t> in_edge;
    for (auto& task : m_tasks)
    {
        in_edge[task.get()] = task->get_dependencies().size();
    }

    std::stack<task_node*> stack;
    for (task_wrapper* root : m_roots)
    {
        stack.push(root);
    }

    std::vector<task_wrapper*> sorted_tasks;
    while (!stack.empty())
    {
        task_node* node = stack.top();
        stack.pop();

        for (task_node* successor : node->get_successors())
        {
            if ((--in_edge[successor]) == 0)
            {
                stack.push(successor);
            }
        }

        sorted_tasks.push_back(static_cast<task_wrapper*>(node));
    }

    assert(sorted_tasks.size() == m_tasks.size() && "cycle in the task graph");

    // Transitive reduction.
    std::unordered_map<task_node*, std::uint8_t> task_flags;
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
                auto iter = std::ranges::find(prev_task_successors, curr_task);
                if (iter != prev_task_successors.end())
                {
                    curr_task->dependencies.push_back(prev_task);
                    prev_task->successors.push_back(curr_task);
                    task_flags[prev_task] = 1;
                }
            }

            if (task_flags[prev_task] != 0)
            {
                for (task_node* dependency : prev_task->get_dependencies())
                {
                    task_flags[dependency] = 1;
                }
            }
        }
    }
}
} // namespace violet