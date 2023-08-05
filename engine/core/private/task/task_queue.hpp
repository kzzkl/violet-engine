#pragma once

#include "core/task/task.hpp"
#include "task/lock_free_queue.hpp"
#include "task/thread_safe_queue.hpp"

namespace violet
{
class task_queue
{
public:
    virtual ~task_queue() = default;

    virtual task_base* pop() = 0;
    virtual void push(task_base* task) = 0;

    virtual void close() = 0;
};

class task_queue_thread_safe : public task_queue
{
public:
    task_queue_thread_safe() : m_close(false) {}

    virtual void push(task_base* task) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(task);
        m_cv.notify_one();
    }

    virtual task_base* pop() override
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_queue.empty() || m_close; });

        if (!m_queue.empty())
        {
            task_base* task = m_queue.front();
            m_queue.pop();
            return task;
        }
        else
        {
            return nullptr;
        }
    }

    virtual void close() override
    {
        m_close = true;
        m_cv.notify_all();
    }

private:
    std::queue<task_base*> m_queue;

    std::condition_variable m_cv;
    std::mutex m_mutex;

    std::atomic<bool> m_close;
};

class task_queue_lock_free : public task_queue
{
public:
    task_queue_lock_free() : m_close(false) {}

    virtual void push(task_base* task) override { m_queue.push(task); }

    virtual task_base* pop() override
    {
        task_base* task = nullptr;
        while (!m_queue.pop(task))
        {
            std::this_thread::yield();

            if (m_close)
                return nullptr;
        }

        return task;
    }

    virtual void close() override { m_close = true; }

private:
    lock_free_queue<task_base*> m_queue;
    std::atomic<bool> m_close;
};
} // namespace violet