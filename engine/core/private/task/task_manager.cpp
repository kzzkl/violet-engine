#include "core/task/task_manager.hpp"
#include "common/log.hpp"
#include "task/task_queue.hpp"
#include "task/thread_pool.hpp"
#include <future>

namespace violet::core
{
task_manager::task_manager(std::size_t thread_count)
{
    if (thread_count == 0)
        thread_count = std::thread::hardware_concurrency();

    m_queues = std::make_unique<task_queue_group>();
    m_thread_pool = std::make_unique<thread_pool>(thread_count);

    auto root_task = schedule(TASK_ROOT, []() {});

    auto logic_start_task = schedule(TASK_GAME_LOGIC_START, []() {});
    logic_start_task->add_dependency(*root_task);

    auto logic_end_task = schedule(TASK_GAME_LOGIC_END, []() {});
    logic_end_task->add_dependency(*logic_start_task);
}

task_manager::~task_manager()
{
}

void task_manager::execute(handle root)
{
    auto count = root->reachable_tasks_count();
    auto done = m_queues->execute(
        &(*root),
        count[to_integer_v<task_type::NONE>] + count[to_integer_v<task_type::MAIN_THREAD>]);

    work_thread_main::run(*m_queues, count[to_integer_v<task_type::MAIN_THREAD>]);
    done.get();
}

void task_manager::run()
{
    m_thread_pool->run(*m_queues);
}

void task_manager::stop()
{
    m_thread_pool->stop();
}

void task_manager::clear()
{
    m_tasks.clear();
}

task_manager::handle task_manager::find(std::string_view name)
{
    auto iter = m_tasks.find(name.data());
    if (iter == m_tasks.end())
        return handle();
    else
        return handle(iter->second.get());
}
} // namespace violet::core