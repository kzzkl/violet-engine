#include "work_thread.hpp"

namespace ash::task
{
work_thread::work_thread() noexcept : m_stop(true), m_queue(nullptr), m_type(task_type::NONE)
{
}

work_thread::work_thread(work_thread&& other) noexcept
    : m_stop(other.m_stop.load()),
      m_queue(other.m_queue),
      m_type(other.m_type),
      m_thread(std::move(other.m_thread))
{
    other.m_queue = nullptr;
    other.m_stop = true;
}

work_thread::~work_thread()
{
    if (!m_stop)
    {
        m_stop = true;

        if (m_queue != nullptr)
            m_queue->notify();

        join();
    }
}

void work_thread::run(task_queue_group& queues)
{
    m_stop = false;
    m_thread = std::thread([this, &queues]() { loop(queues); });
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

work_thread& work_thread::operator=(work_thread&& other) noexcept
{
    m_stop = other.m_stop.load();
    m_queue = other.m_queue;
    m_type = other.m_type;
    m_thread = std::move(other.m_thread);

    other.m_queue = nullptr;

    return *this;
}

void work_thread::loop(task_queue_group& queues)
{
    m_queue = &queues[m_type];
    std::vector<task*> next_tasks;

    while (!m_stop)
    {
        task* current_task = m_queue->pop();
        if (current_task)
        {
            current_task->execute_and_get_next_tasks(next_tasks);
            for (task* next_task : next_tasks)
                queues[next_task->get_type()].push(next_task);
            next_tasks.clear();

            queues.notify_task_completion();
        }
        else
        {
            m_queue->wait_task([this]() { return m_stop.load(); });
        }
    }
    m_queue = nullptr;
}

void work_thread_main::run(task_queue_group& queues, std::size_t task_count)
{
    std::vector<task*> next_tasks;
    while (task_count != 0)
    {
        task* current_task = queues[task_type::MAIN_THREAD].pop();
        if (current_task)
        {
            current_task->execute_and_get_next_tasks(next_tasks);
            for (task* next_task : next_tasks)
                queues[next_task->get_type()].push(next_task);
            next_tasks.clear();

            queues.notify_task_completion();

            --task_count;
        }
        else
        {
            queues[task_type::MAIN_THREAD].wait_task();
        }
    }
}
} // namespace ash::task