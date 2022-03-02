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
    handle schedule(std::string_view name, Callable callable)
    {
        return do_schedule<task_wrapper<Callable>>(name, callable);
    }

    template <typename Callable>
    void schedule_before(std::string_view name, Callable callable)
    {
        std::unique_ptr<task> task = std::make_unique<task_wrapper<Callable>>(name, callable);
        m_before_tasks.push_back(std::move(task));
    }

    template <typename Callable>
    void schedule_after(std::string_view name, Callable callable)
    {
        std::unique_ptr<task> task = std::make_unique<task_wrapper<Callable>>(name, callable);
        m_after_tasks.push_back(std::move(task));
    }

    void run(handle root);

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

    std::vector<std::unique_ptr<task>> m_before_tasks;
    std::vector<std::unique_ptr<task>> m_after_tasks;
    std::unordered_map<std::string, std::unique_ptr<task>> m_tasks;

    task_queue m_queue;
    thread_pool m_thread_pool;

    std::atomic<bool> m_stop;
};
} // namespace ash::task