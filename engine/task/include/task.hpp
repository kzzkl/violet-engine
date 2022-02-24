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

    std::vector<task*> execute_and_get_next_tasks();
    void add_dependency(task& dependency);

    std::string_view get_name() const { return m_name; }

private:
    std::vector<task*> m_next;

    std::atomic<uint8_t> m_dependency_counter;
    uint8_t m_num_dependency;

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