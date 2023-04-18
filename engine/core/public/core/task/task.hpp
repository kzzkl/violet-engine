#pragma once

#include <atomic>
#include <functional>
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

class task
{
public:
    task(std::string_view name, std::size_t index, task_option option);
    virtual ~task() = default;

    std::vector<task*> execute();
    std::vector<task*> visit();

    std::string_view get_name() const noexcept { return m_name; }
    std::size_t get_index() const noexcept { return m_index; }
    std::size_t get_option() const noexcept { return m_option; }

private:
    template <typename... Args>
    friend class task_graph;

    virtual void execute_impl() {}

    std::vector<task*> m_dependents;
    std::vector<task*> m_successors;

    std::atomic<std::uint32_t> m_uncompleted_dependency_count;

    std::string m_name;
    std::size_t m_index;
    task_option m_option;
};

template <typename... Args>
class task_graph
{
public:
    class root_task : public task
    {
    public:
        root_task(std::string_view name, task_graph* graph)
            : task(name, 0, TASK_OPTION_NONE),
              m_graph(graph)
        {
        }

    private:
        virtual void execute_impl() override { m_graph->on_task_complete(); }

        task_graph* m_graph;
    };

    class task_wrapper : public task
    {
    public:
        task_wrapper(
            std::string_view name,
            std::size_t index,
            task_option option,
            task_graph* graph)
            : task(name, index, option),
              m_graph(graph)
        {
        }

        template <typename F>
        task_wrapper(
            std::string_view name,
            std::size_t index,
            task_option option,
            task_graph* graph,
            F&& callable)
            : task(name, index, option),
              m_graph(graph),
              m_callable(std::forward<F>(callable))
        {
        }

    private:
        virtual void execute_impl() override
        {
            if constexpr (sizeof...(Args) == 0)
                m_callable();
            else
                std::apply(m_callable, m_graph->m_parameter);
            m_graph->on_task_complete();
        }

        task_graph* m_graph;
        std::function<void(Args...)> m_callable;
    };

    using parameter_type = std::tuple<Args...>;

public:
    task_graph() : m_incomplete_count(1)
    {
        m_tasks.push_back(std::make_unique<root_task>("root", this));
        m_accessible_tasks.push_back(m_tasks[0].get());
    }

    ~task_graph() {}

    template <typename... Args>
    std::future<void> prepare(const Args&... args)
    {
        m_incomplete_count = static_cast<std::uint32_t>(m_accessible_tasks.size());
        m_promise = std::promise<void>();
        std::future<void> result = m_promise.get_future();
        if (m_incomplete_count == 1)
            m_promise.set_value();
        else
            m_parameter = std::make_tuple(args...);

        return result;
    }

    template <typename F>
    task* add_task(
        std::string_view name,
        F&& functor,
        task_option option = TASK_OPTION_NONE,
        bool link_root = true)
    {
        std::lock_guard<std::mutex> lg(m_lock);

        std::size_t index = 0;
        if (m_free.empty())
        {
            index = m_tasks.size();
            m_tasks.push_back(nullptr);
        }
        else
        {
            index = m_free.front();
            m_free.pop();
        }

        std::unique_ptr<task> new_task =
            std::make_unique<task_wrapper>(name, index, option, this, std::forward<F>(functor));
        task* result = new_task.get();

        m_tasks[index] = std::move(new_task);

        if (link_root)
        {
            get_root()->m_successors.push_back(result);
            result->m_dependents.push_back(get_root());

            on_topology_change();
        }

        return result;
    }

    void remove_task(task* root, bool remove_successor = true)
    {
        if (!is_valid_task(root))
            return;

        std::lock_guard<std::mutex> lg(m_lock);

        std::vector<task*> removed_tasks;
        std::queue<task*> bfs;
        bfs.push(root);
        while (!bfs.empty())
        {
            task* temp = bfs.front();
            bfs.pop();
            removed_tasks.push_back(temp);

            for (task* dependent : temp->m_dependents)
            {
                auto iter =
                    std::find(dependent->m_successors.begin(), dependent->m_successors.end(), temp);
                if (iter != dependent->m_successors.end())
                {
                    std::swap(*iter, dependent->m_successors.back());
                    dependent->m_successors.pop_back();
                }
            }

            if (!remove_successor)
            {
                for (task* successor : temp->m_successors)
                {
                    auto iter = std::find(
                        successor->m_dependents.begin(),
                        successor->m_dependents.end(),
                        temp);
                    if (iter != successor->m_dependents.end())
                    {
                        std::swap(*iter, successor->m_dependents.back());
                        successor->m_dependents.pop_back();
                    }
                }
                break;
            }
            else
            {
                for (task* successor : temp->m_successors)
                    bfs.push(successor);
            }
        }

        for (task* removed_task : removed_tasks)
        {
            m_free.push(removed_task->get_index());
            m_tasks[removed_task->get_index()] = nullptr;
        }

        on_topology_change();
    }

    void link(task* before, task* after)
    {
        if (!is_valid_task(before) || !is_valid_task(after))
            return;

        std::lock_guard<std::mutex> lg(m_lock);

        before->m_successors.push_back(after);
        after->m_dependents.push_back(before);

        on_topology_change();
    }

    void unlink(task* before, task* after)
    {
        if (!is_valid_task(before) || !is_valid_task(after))
            return;

        std::lock_guard<std::mutex> lg(m_lock);

        auto before_iter =
            std::find(before->m_successors.begin(), before->m_successors.end(), after);
        if (before_iter != before->m_successors.end())
        {
            std::swap(*before_iter, before->m_successors.back());
            before->m_successors.pop_back();
        }

        auto after_iter = std::find(after->m_dependents.begin(), after->m_dependents.end(), before);
        if (after_iter != after->m_dependents.end())
        {
            std::swap(*after_iter, after->m_dependents.back());
            after->m_dependents.pop_back();
        }

        on_topology_change();
    }

    task* get_root() const noexcept { return m_tasks[0].get(); }

    std::size_t get_task_count(task_option option)
    {
        if (option == TASK_OPTION_NONE)
            return m_accessible_tasks.size();

        std::size_t result = 0;
        for (task* task : m_accessible_tasks)
        {
            if ((task->get_option() & option) == option)
                ++result;
        }
        return result;
    }

private:
    friend class task_executor;

    void on_topology_change()
    {
        for (auto& task : m_tasks)
        {
            if (task != nullptr)
                task->m_uncompleted_dependency_count =
                    static_cast<std::uint32_t>(task->m_dependents.size());
        }

        m_accessible_tasks = get_root()->visit();
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

    bool is_valid_task(task* task) { return m_tasks[task->get_index()].get() == task; }

    std::mutex m_lock;
    std::promise<void> m_promise;

    std::vector<std::unique_ptr<task>> m_tasks;
    std::queue<std::size_t> m_free;

    std::vector<task*> m_accessible_tasks;
    std::atomic<std::uint32_t> m_incomplete_count;

    parameter_type m_parameter;
};
} // namespace violet