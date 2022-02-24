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
    task_handle();
    task_handle(task_manager* owner, std::size_t index);

    std::size_t operator-(const task_handle& other) { return m_index - other.m_index; }

    task_handle& operator++()
    {
        ++m_index;
        return *this;
    }

    task_handle operator++(int)
    {
        task_handle result = *this;
        ++m_index;
        return result;
    }

    bool operator==(const task_handle& other) const
    {
        return m_owner == other.m_owner && m_index == other.m_index;
    }

    bool operator!=(const task_handle& other) const { return !operator==(other); }

    task& operator*() { return *operator->(); }
    task* operator->();

private:
    task_manager* m_owner;
    std::size_t m_index;
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
        return schedule_task<task_wrapper<Callable>>(name, callable);
    }

    void run();
    void stop();

    handle get_root();

private:
    friend class handle;

    template <derived_from_task T, typename... Args>
    handle schedule_task(Args&&... args)
    {
        handle result(this, m_tasks.size());

        std::unique_ptr<task> task = std::make_unique<T>(std::forward<Args>(args)...);
        m_tasks.push_back(std::move(task));

        return result;
    }

    handle m_root;

    std::vector<std::unique_ptr<task>> m_tasks;
    task_queue m_queue;

    thread_pool m_thread_pool;
};
} // namespace ash::task