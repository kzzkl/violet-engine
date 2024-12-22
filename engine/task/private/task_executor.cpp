#include "task/task_executor.hpp"

namespace violet
{
class task_executor::thread_pool
{
public:
    thread_pool(std::size_t thread_count)
        : m_threads(thread_count)
    {
    }

    ~thread_pool()
    {
        join();
    }

    template <typename Functor>
    void run(Functor functor)
    {
        for (auto& thread : m_threads)
        {
            thread = std::thread(functor);
        }
    }

    void join()
    {
        for (auto& thread : m_threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

private:
    std::vector<std::thread> m_threads;
};

task_executor::task_executor()
    : m_stop(true)
{
}

task_executor::~task_executor()
{
    stop();
}

void task_executor::run(std::size_t thread_count)
{
    if (!m_stop)
    {
        return;
    }

    m_stop = false;

    if (thread_count == 0)
    {
        thread_count = std::thread::hardware_concurrency();
    }

    m_thread_pool = std::make_unique<thread_pool>(thread_count);
    m_thread_pool->run(
        [this]()
        {
            while (true)
            {
                task_wrapper* current = m_worker_thread_queue.pop();
                if (!current)
                {
                    break;
                }

                current->execute();
                current->get_graph()->notify_task_complete();

                on_task_completed(current);
            }
        });
}

void task_executor::stop()
{
    if (m_stop)
    {
        return;
    }

    m_stop = true;

    m_main_thread_queue.close();
    m_worker_thread_queue.close();

    m_thread_pool->join();
    m_thread_pool = nullptr;
}

void task_executor::execute_task(task_wrapper* task)
{
    if (task->is_empty())
    {
        on_task_completed(task);
        return;
    }

    if (task->get_options() & TASK_OPTION_MAIN_THREAD)
    {
        m_main_thread_queue.push(task);
    }
    else
    {
        m_worker_thread_queue.push(task);
    }
}

void task_executor::execute_main_thread_task(std::size_t task_count)
{
    while (task_count > 0)
    {
        task_wrapper* current = m_main_thread_queue.pop();
        if (!current)
        {
            break;
        }

        current->execute();
        current->get_graph()->notify_task_complete();

        for (task_wrapper* successor : current->successors)
        {
            execute_task(successor);
        }

        --task_count;
    }
}

void task_executor::on_task_completed(task_wrapper* task)
{
    for (task_wrapper* successor : task->successors)
    {
        successor->uncompleted_dependency_count.fetch_sub(1);
        std::uint32_t expected = 0;

        if (successor->uncompleted_dependency_count.compare_exchange_strong(
                expected,
                static_cast<std::uint32_t>(successor->dependents.size())))
        {
            execute_task(successor);
        }
    }
}
} // namespace violet