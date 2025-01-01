#pragma once

#include "task/task_group.hpp"
#include <future>
#include <memory>

namespace violet
{
class task_wrapper : public task
{
public:
    task_wrapper(task_graph* graph) noexcept
        : task(graph)
    {
    }

    std::vector<task_wrapper*> dependents;
    std::vector<task_wrapper*> successors;

    std::atomic<std::uint32_t> uncompleted_dependency_count{0};
};

class task_graph
{
public:
    task_graph() noexcept;
    virtual ~task_graph() = default;

    task& add_task()
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

    std::future<void> reset() noexcept;

    const std::vector<task_wrapper*>& get_root_tasks() const noexcept
    {
        return m_roots;
    }

    task& get_task(std::string_view name) const
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

    task_group& get_group(std::string_view name) const
    {
        for (const auto& t : m_groups)
        {
            if (t->get_name() == name)
            {
                return *t;
            }
        }

        throw std::runtime_error("Task not found");
    }

    std::size_t get_task_count() const noexcept;
    std::size_t get_main_thread_task_count() const noexcept
    {
        return m_main_thread_task_count;
    }

    void notify_task_complete();

    void notify_task_change()
    {
        m_dirty = true;
    }

    void print_graph();

    auto& get_tasks()
    {
        compile();
        return m_tasks;
    }

private:
    void compile();
    void transitive_reduction();

    std::vector<std::unique_ptr<task_group>> m_groups;
    std::vector<std::unique_ptr<task_wrapper>> m_tasks;

    std::vector<task_wrapper*> m_roots;

    bool m_dirty;

    std::atomic<std::uint32_t> m_incomplete_count;

    std::size_t m_main_thread_task_count{0};
    std::size_t m_worker_thread_task_count{0};

    std::promise<void> m_promise;
};
} // namespace violet