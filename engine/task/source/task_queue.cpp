#include "task_queue.hpp"

namespace ash::task
{
task* task_queue::pop()
{
    task* result = nullptr;
    m_queue.pop(result);

    return result;
}

void task_queue::push(task* t)
{
    m_queue.push(t);
    m_cv.notify_one();
}

bool task_queue::empty() const
{
    return m_queue.empty();
}

void task_queue::notify()
{
    m_cv.notify_all();
}

void task_queue::wait_task(std::function<bool()> exit)
{
    std::unique_lock<std::mutex> ul(m_lock);
    m_cv.wait(ul, [this, &exit]() { return !empty() || exit(); });
}
} // namespace ash::task