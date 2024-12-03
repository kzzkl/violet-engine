#pragma once

#include "task/task.hpp"

namespace violet
{
class task_group
{
public:
    static constexpr std::string_view group_begin_suffix = " - Begin";
    static constexpr std::string_view group_end_suffix = " - End";

    task_group(task_graph* graph);

    task_group& set_name(std::string_view name)
    {
        m_name = name;

        m_begin->set_name(m_name + group_begin_suffix.data());
        m_end->set_name(m_name + group_end_suffix.data());

        return *this;
    }

    std::string_view get_name() const noexcept
    {
        return m_name;
    }

    task_group& set_group(task_group& group);

    task& get_begin_task()
    {
        return *m_begin;
    }

    task& get_end_task()
    {
        return *m_end;
    }

    template <typename... Args>
    task_group& add_dependency(Args&... predecessor)
    {
        (add_dependency_impl(predecessor), ...);
        return *this;
    }

private:
    void add_dependency_impl(task& dependency);
    void add_dependency_impl(task_group& dependency);

    std::string m_name;
    task_graph* m_graph{nullptr};
    task_group* m_group{nullptr};

    task* m_begin{nullptr};
    task* m_end{nullptr};
};
} // namespace violet