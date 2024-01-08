#include "task/task.hpp"
#include <queue>

namespace violet
{
task_base::task_base(task_graph_base* graph) noexcept
    : m_uncompleted_dependency_count(0),
      m_graph(graph)
{
}

task_base::~task_base()
{
}

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

void task_base::add_successor(task_base* successor)
{
    m_successors.push_back(successor);
    successor->m_dependents.push_back(this);
    ++successor->m_uncompleted_dependency_count;
}

task_graph_base::task_graph_base() noexcept : m_dirty(false), m_incomplete_count(0)
{
}

std::future<void> task_graph_base::reset(task_base* root) noexcept
{
    if (m_dirty)
    {
        m_accessible_tasks = root->visit();
        m_dirty = false;
    }
    m_incomplete_count = m_accessible_tasks.size();
    m_promise = std::promise<void>();

    return m_promise.get_future();
}

void task_graph_base::on_task_complete()
{
    m_incomplete_count.fetch_sub(1);

    std::uint32_t expected = 0;
    if (m_incomplete_count.compare_exchange_strong(
            expected,
            static_cast<std::uint32_t>(m_accessible_tasks.size())))
        m_promise.set_value();
}

std::size_t task_graph_base::get_task_count() const noexcept
{
    return m_accessible_tasks.size();
}
} // namespace violet