#pragma once

#include "task/task_block.hpp"
#include <functional>
#include <string>
#include <vector>

namespace violet
{
class task_node;
class task_group;

template <typename T>
concept task_node_type = std::is_same_v<T, task_node> || std::is_same_v<T, task_group>;

class task_graph;
class task_node
{
public:
    task_node(task_graph* graph) noexcept;
    virtual ~task_node();

    task_node& set_name(std::string_view name)
    {
        m_name = name;
        return *this;
    }

    const std::string& get_name() const noexcept
    {
        return m_name;
    }

    task_node& set_affinity(task_affinity affinity) noexcept
    {
        m_affinity = affinity;
        return *this;
    }

    task_affinity get_affinity() const noexcept
    {
        return m_affinity;
    }

    task_node& set_group(task_group& group);

    template <typename Functor>
    task_node& set_execute(Functor functor)
    {
        m_function = functor;
        return *this;
    }

    template <task_node_type... Args>
    task_node& add_dependency(Args&... predecessor)
    {
        (add_dependency_impl(predecessor), ...);
        return *this;
    }

    const std::vector<task_node*>& get_dependencies() const noexcept
    {
        return m_dependencies;
    }

    const std::vector<task_node*>& get_successors() const noexcept
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
    void add_dependency_impl(task_node& dependency);
    void add_dependency_impl(task_group& dependency);

    std::string m_name;

    task_affinity m_affinity{TASK_AFFINITY_ANY};

    task_graph* m_graph{nullptr};
    task_group* m_group{nullptr};

    std::function<void()> m_function;

    std::vector<task_node*> m_dependencies;
    std::vector<task_node*> m_successors;

    friend class task_graph;
};
} // namespace violet