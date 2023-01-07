#pragma once

#include <array>
#include <atomic>
#include <string_view>
#include <vector>

namespace violet::task
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
    void mark_dependency_dirty();

    void add_next_task(task& task);
    void remove_next_task(task& task);

    void reset_counter();
    void reset_counter_and_get_reachable_tasks(std::vector<task*>& next_tasks);

    std::size_t m_current_index;
    std::size_t m_next_index;
    std::array<std::vector<task*>, 2> m_next_task_list;

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
} // namespace violet::task