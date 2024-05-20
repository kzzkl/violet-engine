#include "task/task_executor.hpp"
#include "common/log.hpp"
#include "task_queue.hpp"

namespace violet
{
class task_executor::thread_pool
{
public:
    thread_pool(std::size_t thread_count) : m_threads(thread_count) {}
    ~thread_pool() { join(); }

    template <typename F>
    void run(F&& functor)
    {
        for (auto& thread : m_threads)
            thread = std::thread(functor);
    }

    void join()
    {
        for (auto& thread : m_threads)
        {
            if (thread.joinable())
                thread.join();
        }
    }

private:
    std::vector<std::thread> m_threads;
};

task_executor::task_executor() : m_stop(true)
{
    m_queues[TASK_TYPE_NORMAL] = std::make_unique<task_queue_thread_safe>();
    m_queues[TASK_TYPE_MAIN_THREAD] = std::make_unique<task_queue_thread_safe>();
}

task_executor::~task_executor()
{
    stop();
}

void task_executor::run(std::size_t thread_count)
{
    if (!m_stop)
        return;

    m_stop = false;

    if (thread_count == 0)
        thread_count = std::thread::hardware_concurrency();

    m_thread_pool = std::make_unique<thread_pool>(thread_count);
    m_thread_pool->run(
        [this]()
        {
            while (true)
            {
                task_base* current = m_queues[TASK_TYPE_NORMAL]->pop();
                if (!current)
                    break;

                for (task_base* successor : current->execute())
                    execute_task(successor);
            }
        });
}

void task_executor::stop()
{
    if (m_stop)
        return;

    m_stop = true;

    for (auto& queue : m_queues)
        queue->close();

    m_thread_pool->join();
    m_thread_pool = nullptr;
}

void task_executor::execute_task(task_base* task)
{
    m_queues[task->get_type()]->push(task);
}

void task_executor::execute_main_thread_task(std::size_t task_count)
{
    while (task_count > 0)
    {
        task_base* current = m_queues[TASK_TYPE_MAIN_THREAD]->pop();
        if (!current)
            break;

        for (task_base* successor : current->execute())
            execute_task(successor);

        --task_count;
    }
}
} // namespace violet