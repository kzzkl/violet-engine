#include "task.hpp"
#include "test_common.hpp"

using namespace violet::task;

namespace violet::test
{
class test_task : public task
{
public:
    using task::task;

    virtual void execute() override {}
};

TEST_CASE("dependencies between tasks", "[task]")
{
    test_task task1("task 1");
    CHECK(task1.get_reachable_tasks_size() == 1);

    test_task task2("task 2");
    task2.add_dependency(task1);
    CHECK(task1.get_reachable_tasks_size() == 2);

    test_task task3("task 3");
    task2.add_dependency(task3);
    CHECK(task1.get_reachable_tasks_size() == 1);

    task3.add_dependency(task1);
    CHECK(task1.get_reachable_tasks_size() == 3);
}
} // namespace violet::test