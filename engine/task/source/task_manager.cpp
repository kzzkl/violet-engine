#include "task_manager.hpp"
#include "log.hpp"
#include <future>

namespace ash::task
{
task_manager::task_manager(std::size_t num_thread) : m_thread_pool(num_thread)
{
}

void task_manager::execute(handle root)
{
    auto count = root->get_reachable_tasks_count();
    auto done = m_queues.execute(
        &(*root),
        count[to_integer_v<task_type::NONE>] + count[to_integer_v<task_type::MAIN_THREAD>]);

    work_thread_main::run(m_queues, count[to_integer_v<task_type::MAIN_THREAD>]);
    done.get();
}

void task_manager::run()
{
    m_thread_pool.run(m_queues);
}

void task_manager::stop()
{
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