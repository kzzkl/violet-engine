#include "task/task.hpp"
#include <queue>

namespace violet
{
task::task() noexcept {}

task::~task() {}

std::vector<task*> task::execute()
{
    m_function();

    std::vector<task*> result;
    for (task* successor : m_successors)
    {
        successor->m_uncompleted_dependency_count.fetch_sub(1);

        std::uint32_t expected = 0;
        if (successor->m_uncompleted_dependency_count.compare_exchange_strong(
                expected, static_cast<std::uint32_t>(successor->m_dependents.size())))
        {
            result.push_back(successor);
        }
    }

    m_taskflow->on_task_complete();

    return result;
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
                    expected, static_cast<std::uint32_t>(successor->m_dependents.size())))
            {
                bfs.push(successor);
            }
        }
    }

    return result;
}

void task::add_predecessor_impl(task& predecessor)
{
    predecessor.add_successor_impl(*this);
}

void task::add_predecessor_impl(std::string_view predecessor_name)
{
    add_predecessor_impl(m_taskflow->get_task(predecessor_name));
}

void task::add_successor_impl(task& successor)
{
    m_successors.push_back(&successor);
    successor.m_dependents.push_back(this);
    ++successor.m_uncompleted_dependency_count;
}

void task::add_successor_impl(std::string_view successor_name)
{
    add_successor_impl(m_taskflow->get_task(successor_name));
}

taskflow::taskflow() noexcept
    : m_dirty(false),
      m_incomplete_count(0)
{
    add_task([]() {}).set_name("root");
}

std::future<void> taskflow::reset() noexcept
{
    if (m_dirty)
    {
        m_accessible_tasks = m_tasks[0]->visit();

        for (task* task : m_accessible_tasks)
        {
            if (task->get_options() & TASK_OPTION_MAIN_THREAD)
            {
                ++m_main_thread_task_count;
            }
        }

        m_dirty = false;
    }
    m_incomplete_count = m_accessible_tasks.size();
    m_promise = std::promise<void>();

    return m_promise.get_future();
}

void taskflow::on_task_complete()
{
    m_incomplete_count.fetch_sub(1);

    std::uint32_t expected = 0;
    if (m_incomplete_count.compare_exchange_strong(
            expected, static_cast<std::uint32_t>(m_accessible_tasks.size())))
    {
        m_promise.set_value();
    }
}

std::size_t taskflow::get_task_count() const noexcept
{
    return m_accessible_tasks.size();
}
} // namespace violet