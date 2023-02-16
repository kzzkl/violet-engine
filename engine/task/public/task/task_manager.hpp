#pragma once

#include "core/context.hpp"
#include "task/task.hpp"
#include <memory>

namespace violet::task
{
static constexpr char TASK_ROOT[] = "root";
static constexpr char TASK_GAME_LOGIC_START[] = "game logic start";
static constexpr char TASK_GAME_LOGIC_END[] = "game logic end";

class task_manager;
class task_handle
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

class task_queue_group;
class thread_pool;

template <typename T>
concept derived_from_task = std::is_base_of<task, T>::value;

class task_manager : public core::system_base
{
public:
    using handle = task_handle;

public:
    task_manager();

    virtual bool initialize(const dictionary& config) override;

    template <typename Callable>
    handle schedule(std::string_view name, Callable callable, task_type type = task_type::NONE)
    {
        return do_schedule<task_wrapper<Callable>>(name, type, callable);
    }

    void execute(handle task);

    void run();
    void stop();

    void clear();

    handle find(std::string_view name);

private:
    template <derived_from_task T, typename... Args>
    handle do_schedule(Args&&... args)
    {
        std::unique_ptr<task> task = std::make_unique<T>(std::forward<Args>(args)...);
        handle result(task.get());

        m_tasks[task->name().data()] = std::move(task);

        return result;
    }

    std::unordered_map<std::string, std::unique_ptr<task>> m_tasks;

    std::unique_ptr<task_queue_group> m_queues;
    std::unique_ptr<thread_pool> m_thread_pool;
};
} // namespace violet::task