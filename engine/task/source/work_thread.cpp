#include "work_thread.hpp"

namespace ash::task
{
work_thread::work_thread() : m_stop(true)
{
}

work_thread::work_thread(work_thread&& other) : m_stop(true)
{
}

work_thread::~work_thread()
{
    if (!m_stop)
    {
        m_stop = true;
        m_queue->notify();
        join();
    }
}

void work_thread::run(task_queue& queue)
{
    m_queue = &queue;
    m_stop = false;
    m_thread = std::thread(&work_thread::loop, this);
}

void work_thread::stop()
{
    m_stop = true;
}

void work_thread::join()
{
    if (m_thread.joinable())
        m_thread.join();
}

void work_thread::loop()
{
    std::vector<task*> next_tasks;
    while (!m_stop)
    {
        task* current_task = m_queue->pop();
        if (current_task)
        {
            current_task->execute_and_get_next_tasks(next_tasks);
            for (task* next_task : next_tasks)
                m_queue->push(next_task);
            next_tasks.clear();

            m_queue->notify_task_completion();
        }
        else
        {
            m_queue->wait_task([this]() { return m_stop.load(); });
        }
    }
}
} // namespace ash::task