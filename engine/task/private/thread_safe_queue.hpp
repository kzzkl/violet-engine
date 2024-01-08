#pragma once

#include <condition_variable>
#include <queue>

namespace violet
{
template <typename T>
class thread_safe_queue
{
public:
    using value_type = T;

public:
    void push(const value_type& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(value);
        m_cv.notify_one();
    }

    bool pop(value_type& value)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_queue.empty(); });
        value = std::move(m_queue.front());
        m_queue.pop();

        return true;
    }

    void notify_all() { m_cv.notify_all(); }

private:
    std::queue<value_type> m_queue;

    std::condition_variable m_cv;
    std::mutex m_mutex;
};
} // namespace violet