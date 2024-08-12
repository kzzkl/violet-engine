#pragma once

#include <array>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <vector>

namespace violet
{
template <typename T>
struct functor_traits : public functor_traits<decltype(&T::operator())>
{
};

template <typename R, typename... Args>
struct functor_traits<R(Args...)>
{
    using argument_type = std::tuple<Args...>;
    using return_type = R;
    using function_type = std::function<R(Args...)>;
};

template <typename R, typename... Args>
struct functor_traits<R (*)(Args...)>
{
    using argument_type = std::tuple<Args...>;
    using return_type = R;
    using function_type = std::function<R(Args...)>;
};

template <typename R, typename C, typename... Args>
struct functor_traits<R (C::*)(Args...) const>
{
    using argument_type = std::tuple<Args...>;
    using return_type = R;
    using function_type = std::function<R(Args...)>;
};

template <typename R, typename C, typename... Args>
struct functor_traits<R (C::*)(Args...)>
{
    using argument_type = std::tuple<Args...>;
    using return_type = R;
    using function_type = std::function<R(Args...)>;
};

enum task_option
{
    TASK_OPTION_NONE = 0,
    TASK_OPTION_MAIN_THREAD = 1 << 0,
};
using task_options = std::uint32_t;

class taskflow;
class task
{
public:
    task() noexcept;
    virtual ~task();

    task& set_name(std::string_view name)
    {
        m_name = name;
        return *this;
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

    template <typename... Args>
    task& add_predecessor(Args&... predecessor)
    {
        (add_predecessor_impl(predecessor), ...);
        return *this;
    }

    template <typename... Args>
    task& add_successor(Args&... successor)
    {
        (add_successor_impl(successor), ...);
        return *this;
    }

    std::vector<task*> execute();
    std::vector<task*> visit();

    bool is_ready() const noexcept
    {
        return m_uncompleted_dependency_count == 0;
    }

private:
    void add_predecessor_impl(task& successor);
    void add_predecessor_impl(std::string_view predecessor_name);
    void add_successor_impl(task& successor);
    void add_successor_impl(std::string_view successor_name);

    std::function<void()> m_function;

    std::vector<task*> m_dependents;
    std::vector<task*> m_successors;

    std::atomic<std::uint32_t> m_uncompleted_dependency_count{0};

    std::string m_name;
    task_options m_options{0};
    taskflow* m_taskflow{nullptr};

    friend class taskflow;
};

class taskflow
{
public:
    taskflow() noexcept;
    virtual ~taskflow() = default;

    template <typename Functor, typename... Args>
    task& add_task(Functor functor)
    {
        auto pointer = std::make_unique<task>();
        pointer->m_taskflow = this;
        pointer->m_function = functor;

        std::lock_guard<std::mutex> lg(m_lock);
        m_tasks.push_back(std::move(pointer));
        m_dirty = true;

        if (m_tasks.size() != 1)
            m_tasks[0]->add_successor(*m_tasks.back());

        return *m_tasks.back();
    }

    std::future<void> reset() noexcept;
    void on_task_complete();

    task& get_root() const noexcept
    {
        return *m_tasks[0];
    }

    task& get_task(std::string_view name) const
    {
        for (auto& task : m_tasks)
        {
            if (task->m_name == name)
            {
                return *task;
            }
        }

        throw std::runtime_error("Task not found");
    }

    std::size_t get_task_count() const noexcept;
    std::size_t get_main_thread_task_count() const noexcept
    {
        return m_main_thread_task_count;
    }

protected:
    bool is_dirty() const noexcept
    {
        return m_dirty;
    }

private:
    std::vector<std::unique_ptr<task>> m_tasks;
    bool m_dirty;

    std::vector<task*> m_accessible_tasks;
    std::atomic<std::uint32_t> m_incomplete_count;
    std::size_t m_main_thread_task_count{0};

    std::promise<void> m_promise;

    std::mutex m_lock;
};
} // namespace violet