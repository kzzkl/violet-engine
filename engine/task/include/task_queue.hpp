#pragma once

#include "lock_free_queue.hpp"
#include "task.hpp"
#include <condition_variable>
#include <functional>
#include <future>

namespace ash::task
{
class task_queue
{
public:
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

    std::future<void> push_root_task(task* t);

    /**
     * @brief Returns whether the task queue is empty.
     *
     * @return Queue is empty
     */
    bool empty() const;

    /**
     * @brief Wake up the internal condition variable.
     *
     * When the wait_task exit condition is met, this function needs to be called to wake up the
     * thread blocked by the condition variable.
     */
    void notify();

    void notify_task_completion(bool force = false);

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

    std::atomic<uint32_t> m_num_remaining_tasks;
    std::promise<void> m_done;
};
} // namespace ash::task