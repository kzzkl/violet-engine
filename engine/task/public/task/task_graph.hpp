#pragma once

#include "task/task_graph/task_group.hpp"
#include "task/task_scheduler.hpp"
#include <algorithm>
#include <memory>

namespace violet
{
class task_wrapper : public task_node
{
public:
    task_wrapper(task_graph* graph) noexcept
        : task_node(graph)
    {
    }

    std::vector<task_wrapper*> dependencies;
    std::vector<task_wrapper*> successors;

    task_event event;
    task_handle handle;
};

class task_graph
{
public:
    task_graph() noexcept;
    virtual ~task_graph() = default;

    task_node& add_task()
    {
        auto node = std::make_unique<task_wrapper>(this);
        m_tasks.push_back(std::move(node));
        m_dirty = true;

        return *m_tasks.back();
    }

    task_group& add_group()
    {
        auto node = std::make_unique<task_group>(this);
        m_groups.push_back(std::move(node));
        m_dirty = true;

        return *m_groups.back();
    }

    task_handle execute(task_scheduler& scheduler);

    void execute_sync(task_scheduler& scheduler)
    {
        execute(scheduler).wait();
    }

    const std::vector<task_wrapper*>& get_root_tasks() const noexcept
    {
        return m_roots;
    }

    task_node& get_task(std::string_view name) const
    {
        for (const auto& t : m_tasks)
        {
            if (t->get_name() == name)
            {
                return *t;
            }
        }

        throw std::runtime_error("Task not found");
    }

    bool has_task(std::string_view name) const
    {
        return std::ranges::any_of(
            m_tasks,
            [name](const auto& t)
            {
                return t->get_name() == name;
            });
    }

    task_group& get_group(std::string_view name) const
    {
        for (const auto& t : m_groups)
        {
            if (t->get_name() == name)
            {
                return *t;
            }
        }

        throw std::runtime_error("Task group not found");
    }

    bool has_group(std::string_view name) const
    {
        return std::ranges::any_of(
            m_groups,
            [name](const auto& t)
            {
                return t->get_name() == name;
            });
    }

    std::uint32_t get_task_count() const noexcept
    {
        return static_cast<std::uint32_t>(m_tasks.size());
    }

    void notify_task_change()
    {
        m_dirty = true;
    }

    void print();

    auto& get_tasks()
    {
        if (m_dirty)
        {
            compile();
            m_dirty = false;
        }

        return m_tasks;
    }

private:
    void compile();
    void transitive_reduction();

    std::vector<std::unique_ptr<task_group>> m_groups;
    std::vector<std::unique_ptr<task_wrapper>> m_tasks;

    std::vector<task_wrapper*> m_roots;

    task_event m_done;

    bool m_executing{false};

    bool m_dirty{false};
};
} // namespace violet