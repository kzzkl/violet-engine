#include "task_queue.hpp"
#include "log.hpp"

namespace ash::task
{
task_queue::task_queue() : m_size(0)
{
}

task* task_queue::pop()
{
    task* result = nullptr;
    if (m_queue.pop(result))
    {
        m_size.fetch_sub(1);
        return result;
    }
    else
    {
        return nullptr;
    }
}

void task_queue::push(task* t)
{
    m_queue.push(t);

    std::lock_guard<std::mutex> lg(m_lock);
    m_size.fetch_add(1);
    m_cv.notify_one();
}

void task_queue::notify()
{
    m_cv.notify_all();
}

void task_queue::wait_task()
{
    std::unique_lock<std::mutex> ul(m_lock);
    m_cv.wait(ul, [this]() { return m_size.load() > 0; });
}

void task_queue::wait_task(std::function<bool()> exit)
{
    std::unique_lock<std::mutex> ul(m_lock);
    m_cv.wait(ul, [this, &exit]() { return m_size.load() > 0 || exit(); });
}

std::future<void> task_queue_group::execute(task* t, std::size_t task_count)
{
    m_remaining_tasks_count = static_cast<uint32_t>(task_count);
    m_done = std::promise<void>();

    switch (t->get_type())
    {
    case task_type::NONE:
        get_queue(task_type::NONE).push(t);
        break;
    case task_type::MAIN_THREAD:
        get_queue(task_type::MAIN_THREAD).push(t);
        break;
    default:
        break;
    }

    return m_done.get_future();
}

void task_queue_group::notify_task_completion(bool force)
{
    while (true)
    {
        uint32_t old_value = m_remaining_tasks_count.load();
        uint32_t new_value = force ? 0 : old_value - 1;

        if (old_value == m_remaining_tasks_count.load())
        {
            if (m_remaining_tasks_count.compare_exchange_weak(old_value, new_value))
            {
                if (old_value != 0 && new_value == 0)
                    m_done.set_value();
                break;
            }
        }
    }
}
} // namespace ash::task