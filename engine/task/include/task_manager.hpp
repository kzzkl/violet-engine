#pragma once

#include "task.hpp"
#include "task_queue.hpp"
#include "thread_pool.hpp"
#include <memory>

namespace ash::task
{
class task_manager;
class TASK_API task_handle
{
public:
    task_handle() : task_handle(nullptr) {}
    task_handle(task* t) : m_task(t) {}

    bool operator==(const task_handle& other) const { return m_task == other.m_task; }
    bool operator!=(const task_handle& other) const { return !operator==(other); }

    task& operator*() { return *operator->(); }
    task* operator->() { return m_task; }

private:
    task* m_task;
};

template <typename T>
concept derived_from_task = std::is_base_of<task, T>::value;

class TASK_API task_manager
{
public:
    using handle = task_handle;

public:
    task_manager(std::size_t num_thread);

    template <typename Callable>
    handle schedule(std::string_view name, Callable callable, task_type type = task_type::NONE)
    {
        return do_schedule<task_wrapper<Callable>>(name, type, callable);
    }

    void execute(handle task);

    void run();
    void stop();

    handle find(std::string_view name);

private:
    friend class handle;

    template <derived_from_task T, typename... Args>
    handle do_schedule(Args&&... args)
    {
        std::unique_ptr<task> task = std::make_unique<T>(std::forward<Args>(args)...);
        handle result(task.get());

        m_tasks[task->get_name().data()] = std::move(task);

        return result;
    }

    std::unordered_map<std::string, std::unique_ptr<task>> m_tasks;

    task_queue_group m_queues;
    thread_pool m_thread_pool;
};
} // namespace ash::task