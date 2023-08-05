#pragma once

#include "core/task/function_tratis.hpp"
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace violet
{
enum task_option : std::uint32_t
{
    TASK_OPTION_NONE = 0,
    TASK_OPTION_MAIN_THREAD = 1
};

class task_graph_base;
class task_base
{
public:
    task_base(task_option option, task_graph_base* graph) : m_option(option), m_graph(graph) {}

    virtual ~task_base() {}

    std::vector<task_base*> execute();
    std::vector<task_base*> visit();

    bool is_ready() const noexcept { return m_uncompleted_dependency_count == 0; }

    std::size_t get_option() const noexcept { return m_option; }

    template <typename... Args>
    static auto resolve(Args&&... args)
    {
        return std::make_tuple(std::forward<Args>(args)...);
    }

protected:
    template <typename T, typename... Args>
    T& make_task(Args&&... args);

    static void add_successor(task_base* dependent, task_base* successor)
    {
        dependent->m_successors.push_back(successor);
        successor->m_dependents.push_back(dependent);
        ++successor->m_uncompleted_dependency_count;
    }

    void add_successor(task_base* successor) { add_successor(this, successor); }

private:
    virtual void execute_impl() {}

    std::vector<task_base*> m_dependents;
    std::vector<task_base*> m_successors;

    std::atomic<std::uint32_t> m_uncompleted_dependency_count;

    task_graph_base* m_graph;
    task_option m_option;
};

class task_graph_base
{
public:
    task_graph_base() : m_dirty(false) {}
    virtual ~task_graph_base() = default;

    template <typename T, typename... Args>
    T& make_task(Args&&... args)
    {
        auto pointer = std::make_unique<T>(std::forward<Args>(args)...);
        auto& result = *pointer;

        std::lock_guard<std::mutex> lg(m_lock);
        m_tasks.push_back(std::move(pointer));
        m_dirty = true;
        return result;
    }

    std::future<void> reset(task_base* root) noexcept
    {
        if (m_dirty)
        {
            m_accessible_tasks = root->visit();
            m_dirty = false;
        }
        m_incomplete_count = m_accessible_tasks.size();
        m_promise = std::promise<void>();

        return m_promise.get_future();
    }

    void on_task_complete()
    {
        m_incomplete_count.fetch_sub(1);

        std::uint32_t expected = 0;
        if (m_incomplete_count.compare_exchange_strong(
                expected,
                static_cast<std::uint32_t>(m_accessible_tasks.size())))
            m_promise.set_value();
    }

    std::size_t get_task_count(int option) const noexcept
    {
        if (option == TASK_OPTION_NONE)
            return m_accessible_tasks.size();

        std::size_t result = 0;
        for (task_base* task : m_accessible_tasks)
        {
            if ((task->get_option() & option) == option)
                ++result;
        }
        return result;
    }

protected:
    bool is_dirty() const noexcept { return m_dirty; }

private:
    std::vector<std::unique_ptr<task_base>> m_tasks;
    bool m_dirty;

    std::vector<task_base*> m_accessible_tasks;
    std::atomic<std::uint32_t> m_incomplete_count;

    std::promise<void> m_promise;

    std::mutex m_lock;
};

template <typename T, typename... Args>
T& task_base::make_task(Args&&... args)
{
    auto& task = m_graph->make_task<T>(std::forward<Args>(args)...);
    task.m_graph = m_graph;
    return task;
}

template <typename T>
class task;

template <typename... Args>
struct next_task
{
};

template <typename R, typename... Args>
struct next_task<R, std::tuple<Args...>>
{
    using type = task<R(Args...)>;
};

template <typename F>
struct next_task<F>
{
    using type = typename next_task<
        typename function_traits<F>::return_type,
        typename function_traits<F>::argument_type>::type;
};

template <typename T>
struct task_value_wrapper
{
    std::unique_ptr<T> value;
};

template <typename R, typename... Args>
class task<R(Args...)> : public task_base
{
public:
    using argument_type = std::tuple<Args...>;
    using result_type = R;

    using argument_wrapper = task_value_wrapper<argument_type>;
    using result_wrapper = task_value_wrapper<result_type>;

public:
    template <typename Functor>
    task(
        Functor functor,
        const argument_wrapper& argument,
        task_option option = TASK_OPTION_NONE,
        task_graph_base* graph = nullptr)
        : task_base(option, graph),
          m_callable(functor),
          m_argument(argument)
    {
    }

    template <typename Functor>
    auto& then(Functor functor)
    {
        using next_type = typename next_task<Functor>::type;

        auto& task = make_task<next_type>(functor, m_result);
        add_successor(&task);
        return task;
    }

private:
    virtual void execute_impl() override
    {
        if (m_result.value != nullptr)
            *m_result.value = std::apply(m_callable, *m_argument.value);
        else
            m_result.value =
                std::make_unique<result_type>(std::apply(m_callable, *m_argument.value));
    }

    std::function<R(Args...)> m_callable;
    const argument_wrapper& m_argument;
    result_wrapper m_result;
};

template <typename R>
class task<R()> : public task_base
{
public:
    using result_type = R;
    using result_wrapper = task_value_wrapper<result_type>;

public:
    template <typename Functor>
    task(Functor functor, task_option option = TASK_OPTION_NONE, task_graph_base* graph = nullptr)
        : task_base(option, graph),
          m_callable(functor)
    {
    }

    template <typename Functor>
    auto& then(Functor functor)
    {
        using next_type = typename next_task<Functor>::type;

        auto& task = make_task<next_type>(functor, m_result);
        add_successor(&task);
        return task;
    }

private:
    virtual void execute_impl() override
    {
        if (m_result.value != nullptr)
            *m_result.value = m_callable();
        else
            m_result.value = std::make_unique<result_type>(m_callable());
    }

    std::function<result_type()> m_callable;
    result_wrapper m_result;
};

template <typename... Args>
class task<void(Args...)> : public task_base
{
public:
    using argument_type = std::tuple<Args...>;
    using argument_wrapper = task_value_wrapper<argument_type>;

public:
    template <typename Functor>
    task(
        Functor functor,
        const argument_wrapper& argument,
        task_option option = TASK_OPTION_NONE,
        task_graph_base* graph = nullptr)
        : task_base(option, graph),
          m_callable(functor),
          m_argument(argument)
    {
    }

    template <typename Functor>
    auto& then(Functor functor)
    {
        using next_type = typename next_task<Functor>::type;

        auto& task = make_task<next_type>(functor);
        add_successor(&task);
        return task;
    }

private:
    virtual void execute_impl() override { std::apply(m_callable, *m_argument.value); }

    std::function<void(Args...)> m_callable;
    const argument_wrapper& m_argument;
};

template <>
class task<void()> : public task_base
{
public:
    template <typename Functor>
    task(Functor functor, task_option option = TASK_OPTION_NONE, task_graph_base* graph = nullptr)
        : task_base(option, graph),
          m_callable(functor)
    {
    }

    template <typename Functor>
    auto& then(Functor functor)
    {
        using next_type = typename next_task<Functor>::type;

        auto& task = make_task<next_type>(functor);
        add_successor(&task);
        return task;
    }

private:
    virtual void execute_impl() override { m_callable(); }

    std::function<void()> m_callable;
};

/*class task_all : public task
{
public:
    task_all(const std::vector<task*>& dependents, task_graph* graph);

    template <typename Functor>
    auto& then(Functor functor)
    {
        auto& task = make_task<task<NextFunctor, task_all>>(f, this);
        add_successor(&task);
        return task;
    }

    std::tuple<> get_result() { return {}; }
};*/

template <typename... Args>
class task_graph : public task_graph_base
{
public:
    class task_root : public task_base
    {
    public:
        using argument_type = std::tuple<Args...>;
        using result_type = argument_type;
        using result_wrapper = task_value_wrapper<result_type>;

    public:
        task_root(task_graph* graph) : task_base(TASK_OPTION_NONE, graph) {}

        template <typename Functor>
        auto& then(Functor f)
        {
            using next_type = typename next_task<Functor>::type;

            if constexpr (sizeof...(Args) != 0)
            {
                auto& task = make_task<next_type>(f, m_result);
                add_successor(&task);
                return task;
            }
            else
            {
                auto& task = make_task<next_type>(f);
                add_successor(&task);
                return task;
            }
        }

        void set_argument(Args&&... args)
        {
            if constexpr (sizeof...(Args) != 0)
            {
                if (m_result.value)
                    *m_result.value = std::make_tuple(std::forward<Args>(args)...);
                else
                    m_result.value =
                        std::make_unique<result_type>(std::make_tuple(std::forward<Args>(args)...));
            }
        }

    private:
        virtual void execute_impl() override {}

        result_wrapper m_result;
    };

public:
    task_graph() : m_root(this) {}

    template <typename Functor>
    auto& then(Functor functor)
    {
        return m_root.then(functor);
    }

    // task_all& all(const std::vector<task*>& dependents);

    void set_argument(Args&&... args) { m_root.set_argument(std::forward<Args>(args)...); }

    task_base* get_root() noexcept { return &m_root; }

    std::future<void> reset() noexcept { return task_graph_base::reset(&m_root); }

private:
    task_root m_root;
};
} // namespace violet