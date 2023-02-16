#include "thread_pool.hpp"

namespace violet::task
{
thread_pool::thread_pool(std::size_t num_thread) : m_queues(nullptr)
{
    m_threads.resize(num_thread);
}

thread_pool::~thread_pool()
{
    stop();
}

void thread_pool::run(task_queue_group& queues)
{
    m_queues = &queues;

    for (work_thread& thread : m_threads)
        thread.run(queues);
}

void thread_pool::stop()
{
    for (work_thread& thread : m_threads)
        thread.stop();

    for (auto& queue : *m_queues)
        queue.notify();

    for (work_thread& thread : m_threads)
        thread.join();

    m_queues->notify_task_completion(true);
}
} // namespace violet::task