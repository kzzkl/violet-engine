#include "core/task/task.hpp"
#include <queue>

namespace violet
{
task::task(std::string_view name, task_type type)
    : m_dependency_count(0),
      m_uncompleted_dependency_count(0),
      m_name(name),
      m_type(type),
      m_current_index(0),
      m_next_index(0)
{
}

task::~task()
{
}

void task::execute_and_get_next_tasks(std::vector<task*>& next_tasks)
{
    execute();
    reset_counter_and_get_reachable_tasks(next_tasks);
}

void task::add_dependency(task& dependency)
{
    mark_dependency_dirty();

    dependency.add_next_task(*this);

    ++m_dependency_count;
    reset_counter();
}

void task::remove_dependency(task& dependency)
{
    mark_dependency_dirty();

    dependency.remove_next_task(*this);

    --m_dependency_count;
    reset_counter();
}

void task::mark_dependency_dirty()
{
    if (m_current_index == m_next_index)
    {
        m_next_index = (m_current_index + 1) % m_next_task_list.size();
        m_next_task_list[m_next_index] = m_next_task_list[m_current_index];
    }
}

void task::add_next_task(task& task)
{
    m_next_task_list[m_next_index].push_back(&task);
}

void task::remove_next_task(task& task)
{
    auto& list = m_next_task_list[m_next_index];
    for (auto iter = list.begin(); iter != list.end(); ++iter)
    {
        if (*iter == &task)
        {
            std::swap(*iter, list.back());
            list.pop_back();
            break;
        }
    }
}

std::array<std::size_t, TASK_TYPE_COUNT> task::reachable_tasks_count()
{
    std::array<std::size_t, TASK_TYPE_COUNT> result = {};

    std::queue<task*> q;
    q.push(this);

    std::vector<task*> next_tasks;
    while (!q.empty())
    {
        task* t = q.front();
        q.pop();

        t->reset_counter_and_get_reachable_tasks(next_tasks);
        for (task* next_task : next_tasks)
            q.push(next_task);

        next_tasks.clear();
        ++result[static_cast<std::size_t>(t->type())];
    }

    return result;
}

void task::reset_counter()
{
    m_current_index = m_next_index;
    m_uncompleted_dependency_count = m_dependency_count;
}

void task::reset_counter_and_get_reachable_tasks(std::vector<task*>& next_tasks)
{
    reset_counter();

    for (task* next : m_next_task_list[m_current_index])
    {
        --next->m_uncompleted_dependency_count;
        if (next->m_uncompleted_dependency_count == 0)
            next_tasks.push_back(next);
    }
}
} // namespace violet