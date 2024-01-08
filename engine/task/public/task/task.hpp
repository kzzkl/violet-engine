#pragma once

#include <functional>
#include <future>
#include <memory>
#include <mutex>
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

class task_graph_base;
class task_base
{
public:
    task_base(task_graph_base* graph = nullptr) noexcept;
    virtual ~task_base();

    std::vector<task_base*> execute();
    std::vector<task_base*> visit();

    bool is_ready() const noexcept { return m_uncompleted_dependency_count == 0; }

protected:
    void add_successor(task_base* successor);

    task_graph_base* get_graph() const noexcept { return m_graph; }

private:
    friend class task_graph_base;

    virtual void execute_impl() {}

    std::vector<task_base*> m_dependents;
    std::vector<task_base*> m_successors;

    std::atomic<std::uint32_t> m_uncompleted_dependency_count;

    task_graph_base* m_graph;
};

class task_graph_base
{
public:
    task_graph_base() noexcept;
    virtual ~task_graph_base() = default;

    template <typename T, typename... Args>
    T& add_task(Args&&... args)
    {
        auto pointer = std::make_unique<T>(std::forward<Args>(args)...);
        pointer->m_graph = this;

        auto& result = *pointer;

        std::lock_guard<std::mutex> lg(m_lock);
        m_tasks.push_back(std::move(pointer));
        m_dirty = true;
        return result;
    }

    std::future<void> reset(task_base* root) noexcept;
    void on_task_complete();

    std::size_t get_task_count() const noexcept;

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

template <typename T>
class task_node;

template <typename... Args>
struct next_task
{
};

template <typename R, typename... Args>
struct next_task<R, std::tuple<Args...>>
{
    using type = task_node<R(Args...)>;
};

template <typename F>
struct next_task<F>
{
    using type = typename next_task<
        typename functor_traits<F>::return_type,
        typename functor_traits<F>::argument_type>::type;
};

template <typename... Args>
class task : public task_base
{
public:
    using result_type = std::tuple<Args...>;

public:
    using task_base::task_base;

    template <typename Functor>
    auto& then(Functor functor)
    {
        using next_type = typename next_task<Functor>::type;

        auto& task = get_graph()->add_task<next_type>(functor, this);
        add_successor(&task);
        return task;
    }

    result_type& get_result() { return *m_result; }

protected:
    void set_result(result_type&& result)
    {
        if (m_result)
            *m_result = result;
        else
            m_result = std::make_unique<result_type>(result);
    }

private:
    std::unique_ptr<result_type> m_result;
};

template <>
class task<> : public task_base
{
public:
    using task_base::task_base;

    template <typename Functor>
    auto& then(Functor functor)
    {
        using next_type = typename next_task<Functor>::type;

        auto& task = get_graph()->add_task<next_type>(functor, this);
        add_successor(&task);
        return task;
    }
};

template <typename T>
struct task_impl_traits
{
};

template <typename... Args>
struct task_impl_traits<std::tuple<Args...>>
{
    using type = task<Args...>;
    static constexpr bool has_result = sizeof...(Args) != 0;
};

template <>
struct task_impl_traits<void>
{
    using type = task<>;
    static constexpr bool has_result = false;
};

template <typename R, typename... Args>
class task_node<R(Args...)> : public task_impl_traits<R>::type
{
public:
    using prev_task_type = task<Args...>;
    using base_type = typename task_impl_traits<R>::type;

public:
    template <typename Functor>
    task_node(Functor functor, prev_task_type* prev_task)
        : m_callable(functor),
          m_prev_task(prev_task)
    {
    }

private:
    virtual void execute_impl() override
    {
        if constexpr (task_impl_traits<R>::has_result)
        {
            if constexpr (sizeof...(Args) != 0)
                base_type::set_result(std::apply(m_callable, m_prev_task->get_result()));
            else
                base_type::set_result(m_callable());
        }
        else
        {
            if constexpr (sizeof...(Args) != 0)
                std::apply(m_callable, m_prev_task->get_result());
            else
                m_callable();
        }
    }

    std::function<R(Args...)> m_callable;
    prev_task_type* m_prev_task;
};

template <typename... Args>
class task_graph : public task_graph_base
{
public:
    class task_root : public task<Args...>
    {
    public:
        using base_type = task<Args...>;

    public:
        task_root(task_graph* graph) : task<Args...>(graph) {}

        template <typename... Ts>
        void set_argument(Ts&&... args)
        {
            if constexpr (sizeof...(Ts) != 0)
                base_type::set_result(std::make_tuple(std::forward<Ts>(args)...));
        }

    private:
        virtual void execute_impl() override {}
    };

public:
    task_graph() : m_root(this) {}

    template <typename... Ts>
    void set_argument(Ts&&... args)
    {
        m_root.set_argument(std::forward<Ts>(args)...);
    }

    task<Args...>& get_root() noexcept { return m_root; }

    std::future<void> reset() noexcept { return task_graph_base::reset(&m_root); }

private:
    task_root m_root;
};
} // namespace violet