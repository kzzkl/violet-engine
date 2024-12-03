#pragma once

#include <functional>
#include <string>
#include <vector>

namespace violet
{
enum task_option
{
    TASK_OPTION_NONE = 0,
    TASK_OPTION_MAIN_THREAD = 1 << 0,
};
using task_options = std::uint32_t;

class task;
class task_group;

template <typename T>
concept task_node = std::is_same_v<T, task> || std::is_same_v<T, task_group>;

class task_graph;
class task
{
public:
    task(task_graph* graph) noexcept;
    virtual ~task();

    task& set_name(std::string_view name)
    {
        m_name = name;
        return *this;
    }

    std::string_view get_name() const noexcept
    {
        return m_name;
    }

    task& set_options(task_options options) noexcept
    {
        m_options = options;
        return *this;
    }

    task_options get_options() const noexcept
    {
        return m_options;
    }

    task& set_group(task_group& group);

    template <typename Functor>
    task& set_execute(Functor functor)
    {
        m_function = functor;
        return *this;
    }

    template <task_node... Args>
    task& add_dependency(Args&... predecessor)
    {
        (add_dependency_impl(predecessor), ...);
        return *this;
    }

    const std::vector<task*>& get_dependents() const noexcept
    {
        return m_dependents;
    }

    const std::vector<task*>& get_successors() const noexcept
    {
        return m_successors;
    }

    void execute()
    {
        if (m_function)
        {
            m_function();
        }
    }

    task_graph* get_graph() const noexcept
    {
        return m_graph;
    }

    bool is_empty() const noexcept
    {
        return !m_function;
    }

private:
    void add_dependency_impl(task& dependency);
    void add_dependency_impl(task_group& dependency);

    std::string m_name;
    task_options m_options{0};
    task_graph* m_graph{nullptr};
    task_group* m_group{nullptr};

    std::function<void()> m_function;

    std::vector<task*> m_dependents;
    std::vector<task*> m_successors;
};
} // namespace violet