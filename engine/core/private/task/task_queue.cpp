#include "task/task_queue.hpp"

namespace violet
{
task_queue::task_queue() : m_exit(false)
{
}

task* task_queue::pop()
{
    task* task = nullptr;
    while (!m_queue.pop(task))
    {
        std::this_thread::yield();

        if (m_exit)
            return nullptr;
    }

    return task;
}

void task_queue::push(task* task)
{
    m_queue.push(task);
}

void task_queue::start()
{
    m_exit = false;
}

void task_queue::stop()
{
    m_exit = true;
}
} // namespace violet