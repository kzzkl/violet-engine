#pragma once

#include "task_queue.hpp"
#include <thread>

namespace ash::task
{
class work_thread
{
public:
    work_thread();
    work_thread(const work_thread&) = delete;
    work_thread(work_thread&& other);

    ~work_thread();

    void run(task_queue& queue);
    void stop();

    void join();

    work_thread& operator=(const work_thread&) = delete;
    work_thread& operator=(work_thread&&) = default;

private:
    void loop();

    std::atomic<bool> m_stop;
    task_queue* m_queue;

    std::thread m_thread;
};
} // namespace ash::task