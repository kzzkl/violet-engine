#include "core/task/task.hpp"

namespace violet
{
task::task(std::string_view name, std::size_t index, task_option option)
    : m_uncompleted_dependency_count(0),
      m_name(name),
      m_index(index),
      m_option(option)
{
}

std::vector<task*> task::execute()
{
    execute_impl();

    for (task* successor : m_successors)
        successor->m_uncompleted_dependency_count.fetch_sub(1);

    m_uncompleted_dependency_count = static_cast<std::uint32_t>(m_dependents.size());

    std::vector<task*> successors;
    for (task* successor : m_successors)
    {
        std::uint32_t expected = 0;
        if (successor->m_uncompleted_dependency_count.compare_exchange_strong(
                expected,
                static_cast<std::uint32_t>(successor->m_dependents.size())))
        {
            successors.push_back(successor);
        }
    }
    return successors;
}

std::vector<task*> task::visit()
{
    std::queue<task*> bfs;
    bfs.push(this);

    std::vector<task*> result;

    while (!bfs.empty())
    {
        task* temp = bfs.front();
        bfs.pop();

        result.push_back(temp);

        for (task* successor : temp->m_successors)
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
} // namespace violet