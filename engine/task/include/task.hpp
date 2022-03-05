#pragma once

#include "task_exports.hpp"
#include <atomic>
#include <string_view>
#include <vector>

namespace ash::task
{
class TASK_API task
{
public:
    task(std::string_view name);
    virtual ~task();

    virtual void execute() = 0;

    void execute_and_get_next_tasks(std::vector<task*>& next_tasks);

    void add_dependency(task& dependency);
    void remove_dependency(task& dependency);

    std::size_t get_reachable_tasks_size();
    std::string_view get_name() const { return m_name; }

private:
    void add_next_task(task& task);
    void remove_next_task(task& task);

    void reset_counter();
    void reset_counter_and_get_reachable_tasks(std::vector<task*>& next_tasks);

    std::vector<task*> m_next;

    uint8_t m_dependency_count;
    std::atomic<uint8_t> m_uncompleted_dependency_count;

    std::string m_name;
};

template <typename Callable>
class task_wrapper : public task
{
public:
    task_wrapper(std::string_view name, Callable callable) : task(name), m_callable(callable) {}

    virtual void execute() override { m_callable(); }

private:
    Callable m_callable;
};
} // namespace ash::task