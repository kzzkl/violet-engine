#include "task.hpp"
#include <queue>

namespace ash::task
{
task::task(std::string_view name)
    : m_num_dependency(0),
      m_num_uncompleted_dependency(0),
      m_name(name)
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
    dependency.add_next_task(*this);

    ++m_num_dependency;
    reset_counter();
}

void task::remove_dependency(task& dependency)
{
    dependency.remove_next_task(*this);

    --m_num_dependency;
    reset_counter();
}

void task::add_next_task(task& task)
{
    m_next.push_back(&task);
}

void task::remove_next_task(task& task)
{
    for (auto iter = m_next.begin(); iter != m_next.end(); ++iter)
    {
        if (*iter == &task)
        {
            std::swap(*iter, m_next.back());
            m_next.pop_back();
            break;
        }
    }
}

std::size_t task::get_reachable_tasks_size()
{
    std::size_t counter = 0;

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
        ++counter;
    }

    return counter;
}

void task::reset_counter()
{
    m_num_uncompleted_dependency = m_num_dependency;
}

void task::reset_counter_and_get_reachable_tasks(std::vector<task*>& next_tasks)
{
    reset_counter();

    for (task* next : m_next)
    {
        --next->m_num_uncompleted_dependency;
        if (next->m_num_uncompleted_dependency == 0)
            next_tasks.push_back(next);
    }
}
} // namespace ash::task