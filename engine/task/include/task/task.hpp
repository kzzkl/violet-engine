#pragma once

#include <array>
#include <atomic>
#include <string_view>
#include <vector>

namespace ash::task
{
enum class task_type : std::uint8_t
{
    NONE,
    MAIN_THREAD
};

template <task_type type>
struct to_integer
{
    static constexpr std::size_t value = static_cast<std::size_t>(type);
};

template <task_type type>
static constexpr std::size_t to_integer_v = to_integer<type>::value;

static constexpr std::size_t TASK_TYPE_COUNT = 2;

class task
{
public:
    task(std::string_view name, task_type type);
    virtual ~task();

    virtual void execute() = 0;

    void execute_and_get_next_tasks(std::vector<task*>& next_tasks);

    void add_dependency(task& dependency);
    void remove_dependency(task& dependency);

    std::array<std::size_t, TASK_TYPE_COUNT> reachable_tasks_count();

    std::string_view name() const noexcept { return m_name; }
    task_type type() const noexcept { return m_type; }

private:
    void add_next_task(task& task);
    void remove_next_task(task& task);

    void reset_counter();
    void reset_counter_and_get_reachable_tasks(std::vector<task*>& next_tasks);

    std::vector<task*> m_next;

    std::uint8_t m_dependency_count;
    std::atomic<std::uint8_t> m_uncompleted_dependency_count;

    std::string m_name;
    task_type m_type;
};

template <typename Callable>
class task_wrapper : public task
{
public:
    task_wrapper(std::string_view name, task_type type, Callable callable)
        : task(name, type),
          m_callable(callable)
    {
    }

    virtual void execute() override { m_callable(); }

private:
    Callable m_callable;
};

class task_group : public task
{
public:
    task_group(std::string_view name, task_type type);

    virtual void execute() override;

    void add(task* task);
    void remove(task* task);

private:
    std::vector<task*> m_tasks;
};
} // namespace ash::task