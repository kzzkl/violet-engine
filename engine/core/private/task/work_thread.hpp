#pragma once

#include "task_queue.hpp"
#include <thread>

namespace violet::core
{
class work_thread
{
public:
    work_thread() noexcept;
    work_thread(const work_thread&) = delete;
    work_thread(work_thread&& other) noexcept;

    ~work_thread();

    void run(task_queue_group& queues);
    void stop();

    void join();

    work_thread& operator=(const work_thread&) = delete;
    work_thread& operator=(work_thread&& other) noexcept;

private:
    void loop(task_queue_group& queues);

    std::atomic<bool> m_stop;

    task_queue* m_queue;

    task_type m_type;
    std::thread m_thread;
};

class work_thread_main
{
public:
    static void run(task_queue_group& queues, std::size_t task_count);
};
} // namespace violet::core