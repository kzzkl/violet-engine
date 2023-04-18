#include "core/task/task_executor.hpp"
#include "common/log.hpp"
#include "task/task_queue.hpp"

namespace violet
{
class task_executor::thread_pool
{
public:
    thread_pool(std::size_t thread_count) : m_threads(thread_count) {}
    ~thread_pool() { join(); }

    template <typename F>
    void run(F functor)
    {
        for (auto& thread : m_threads)
            thread = std::thread(functor);
    }

    void join()
    {
        for (auto& thread : m_threads)
        {
            if (thread.joinable())
                thread.join();
        }
    }

private:
    std::vector<std::thread> m_threads;
};

task_executor::task_executor(std::size_t thread_count)
{
    if (thread_count == 0)
        thread_count = std::thread::hardware_concurrency();

    m_queue = std::make_unique<task_queue>();
    m_main_thread_queue = std::make_unique<task_queue>();

    m_thread_pool = std::make_unique<thread_pool>(thread_count);
    m_thread_pool->run([this]() {
        while (true)
        {
            task* current = m_queue->pop();
            if (!current)
                break;

            for (task* successor : current->execute())
                execute_task(successor);
        }
    });
}

task_executor::~task_executor()
{
    stop();
}

void task_executor::stop()
{
    m_queue->stop();
    m_main_thread_queue->stop();
    m_thread_pool->join();
}

void task_executor::execute_task(task* task)
{
    if ((task->get_option() & TASK_OPTION_MAIN_THREAD) == TASK_OPTION_MAIN_THREAD)
        m_main_thread_queue->push(task);
    else
        m_queue->push(task);
}

void task_executor::execute_main_thread_task(std::size_t task_count)
{
    while (task_count > 0)
    {
        task* current = m_main_thread_queue->pop();
        if (!current)
            break;

        for (task* successor : current->execute())
            execute_task(successor);

        --task_count;
    }
}
} // namespace violet