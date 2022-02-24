#include "task.hpp"

namespace ash::task
{
task::task(std::string_view name) : m_dependency_counter(0), m_num_dependency(0), m_name(name)
{
}

task::~task()
{
}

std::vector<task*> task::execute_and_get_next_tasks()
{
    m_dependency_counter = m_num_dependency;

    execute();

    std::vector<task*> next_tasks;

    for (task* next : m_next)
    {
        --next->m_dependency_counter;
        if (next->m_dependency_counter == 0)
            next_tasks.push_back(next);
    }

    return next_tasks;
}

void task::add_dependency(task& dependency)
{
    ++m_num_dependency;
    m_dependency_counter = m_num_dependency;

    dependency.m_next.push_back(this);
}
} // namespace ash::task