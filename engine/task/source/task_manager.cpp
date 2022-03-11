#include "task_manager.hpp"
#include "log.hpp"

namespace ash::task
{
task_manager::task_manager(std::size_t num_thread) : m_thread_pool(num_thread), m_stop(true)
{
}

void task_manager::run(handle root)
{
    if (!m_stop)
    {
        log::warn("Task Manager is already running.");
        return;
    }

    m_stop = false;
    m_thread_pool.run(m_queue);

    while (!m_stop)
    {
        for (auto& t : m_before_tasks)
            t->execute();

        auto done = m_queue.push_root_task(&(*root));
        done.get();

        for (auto& t : m_after_tasks)
            t->execute();
    }
}

void task_manager::stop()
{
    m_stop = true;
    m_thread_pool.stop();
}

task_manager::handle task_manager::find(std::string_view name)
{
    auto iter = m_tasks.find(name.data());
    if (iter == m_tasks.end())
        return handle();
    else
        return handle(iter->second.get());
}
} // namespace ash::task