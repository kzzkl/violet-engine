#pragma once

#include "task/work_thread.hpp"

namespace violet::task
{
class thread_pool
{
public:
    thread_pool(std::size_t num_thread);
    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = default;
    ~thread_pool();

    void run(task_queue_group& queues);
    void stop();

    thread_pool& operator=(const thread_pool&) = delete;

private:
    std::vector<work_thread> m_threads;
    task_queue_group* m_queues;
};
} // namespace violet::task