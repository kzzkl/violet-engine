#pragma once

#include "task/lock_free_queue.hpp"
#include <mutex>
#include <queue>

namespace violet
{
template <typename T>
class task_queue_thread_safe
{
public:
    using task_type = T;

    task_queue_thread_safe()
        : m_close(false)
    {
    }

    void push(task_type* task)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(task);
        m_cv.notify_one();
    }

    task_type* pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(
            lock,
            [this]
            {
                return !m_queue.empty() || m_close;
            });

        if (!m_queue.empty())
        {
            task_type* task = m_queue.front();
            m_queue.pop();
            return task;
        }

        return nullptr;
    }

    void close()
    {
        m_close = true;
        m_cv.notify_all();
    }

private:
    std::queue<task_type*> m_queue;

    std::condition_variable m_cv;
    std::mutex m_mutex;

    std::atomic<bool> m_close;
};

template <typename T>
class task_queue_lock_free
{
public:
    using task_type = T;

    task_queue_lock_free()
        : m_close(false)
    {
    }

    void push(task_type* task)
    {
        m_queue.push(task);
    }

    task_type* pop()
    {
        task_type* task = nullptr;
        while (!m_queue.pop(task))
        {
            std::this_thread::yield();

            if (m_close)
            {
                return nullptr;
            }
        }

        return task;
    }

    void close()
    {
        m_close = true;
    }

private:
    lock_free_queue<task_type*> m_queue;
    std::atomic<bool> m_close;
};
} // namespace violet