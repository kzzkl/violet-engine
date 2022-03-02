#include "thread_pool.hpp"

namespace ash::task
{
thread_pool::thread_pool(std::size_t num_thread) : m_queue(nullptr)
{
    for (std::size_t i = 0; i < num_thread; ++i)
        m_threads.emplace_back();
}

thread_pool::~thread_pool()
{
    stop();
}

void thread_pool::run(task_queue& queue)
{
    m_queue = &queue;

    for (work_thread& thread : m_threads)
        thread.run(queue);
}

void thread_pool::stop()
{
    for (work_thread& thread : m_threads)
        thread.stop();

    m_queue->notify();

    for (work_thread& thread : m_threads)
        thread.join();

    m_queue->notify_task_completion(true);
}
} // namespace ash::task