#include "core/task/task.hpp"

namespace violet
{
std::vector<task_base*> task_base::execute()
{
    execute_impl();

    std::vector<task_base*> result;
    for (task_base* successor : m_successors)
    {
        successor->m_uncompleted_dependency_count.fetch_sub(1);

        std::uint32_t expected = 0;
        if (successor->m_uncompleted_dependency_count.compare_exchange_strong(
                expected,
                static_cast<std::uint32_t>(successor->m_dependents.size())))
        {
            result.push_back(successor);
        }
    }

    m_graph->on_task_complete();

    return result;
}

std::vector<task_base*> task_base::visit()
{
    std::queue<task_base*> bfs;
    bfs.push(this);

    std::vector<task_base*> result;

    while (!bfs.empty())
    {
        task_base* temp = bfs.front();
        bfs.pop();

        result.push_back(temp);

        for (task_base* successor : temp->m_successors)
        {
            --successor->m_uncompleted_dependency_count;

            std::uint32_t expected = 0;
            if (successor->m_uncompleted_dependency_count.compare_exchange_strong(
                    expected,
                    static_cast<std::uint32_t>(successor->m_dependents.size())))
            {
                bfs.push(successor);
            }
        }
    }

    return result;
}

/*task_all::task_all(const std::vector<task_base*>& dependents, task_graph* graph)
    : task(TASK_OPTION_NONE, graph)
{
    for (task_base* dependent : dependents)
    {
        task::add_successor(dependent, this);
    }
}

task_all& task_graph::all(const std::vector<task_base*>& dependents)
{
    return make_task<task_all>(dependents, this);
}*/
} // namespace violet