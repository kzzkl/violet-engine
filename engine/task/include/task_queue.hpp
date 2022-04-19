#pragma once

#include "lock_free_queue.hpp"
#include "task.hpp"
#include <array>
#include <condition_variable>
#include <functional>
#include <future>

namespace ash::task
{
class task_queue
{
public:
    task_queue();

    /**
     * @brief Remove the task from the task queue. Returns the task at the head of the queue when
     * there are tasks in the queue, otherwise returns nullptr.
     *
     * @return task at the head of the queue
     */
    task* pop();

    /**
     * @brief Add the task to the end of the task queue.
     *
     * @param t task
     */
    void push(task* t);

    /**
     * @brief Wake up the internal condition variable.
     *
     * When the wait_task exit condition is met, this function needs to be called to wake up the
     * thread blocked by the condition variable.
     */
    void notify();

    void wait_task();

    /**
     * @brief Block until a new task is added or an exit condition is met.
     *
     * @param exit Exit conditions
     */
    void wait_task(std::function<bool()> exit);

private:
    lock_free_queue<task*> m_queue;

    std::condition_variable m_cv;
    std::mutex m_lock;

    std::atomic<std::size_t> m_size;
};

class task_queue_group
{
public:
    auto begin() noexcept { return m_queues.begin(); }
    auto end() noexcept { return m_queues.end(); }

    std::future<void> execute(task* t, std::size_t task_count);
    void notify_task_completion(bool force = false);

    task_queue& queue(task_type type) { return m_queues[static_cast<std::size_t>(type)]; }
    task_queue& operator[](task_type type) { return queue(type); }

private:
    std::array<task_queue, TASK_TYPE_COUNT> m_queues;

    std::promise<void> m_done;
    std::atomic<std::uint32_t> m_remaining_tasks_count;
};
} // namespace ash::task