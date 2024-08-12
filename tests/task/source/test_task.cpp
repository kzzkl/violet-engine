#include "task/task.hpp"
#include "task/task_executor.hpp"
#include "test_common.hpp"
#include <iostream>
#include <queue>

namespace violet::test
{
TEST_CASE("dependencies between tasks", "[task]")
{
    int num = 0;
    std::cout << "main thread: " << std::this_thread::get_id() << std::endl;

    taskflow taskflow;

    task& task_1 = taskflow.add_task(
                               [&]()
                               {
                                   std::cout << "task 1: " << std::this_thread::get_id() << std::endl;
                                   num = 10;
                               })
                       .set_name("task 1")
                       .set_options(TASK_OPTION_MAIN_THREAD);

    task& task_2 = taskflow.add_task(
                               [&]()
                               {
                                   std::cout << "task 2: " << std::this_thread::get_id() << std::endl;
                                   CHECK(num == 10);
                                   num = 20;
                               })
                       .set_name("task 2")
                       .add_predecessor(task_1);

    task& task_3 = taskflow.add_task(
                               [&]()
                               {
                                   std::cout << "task 3: " << std::this_thread::get_id() << std::endl;
                                   CHECK(num == 20);
                                   num = 30;
                               })
                       .set_name("task 3")
                       .add_predecessor(task_2)
                       .set_options(TASK_OPTION_MAIN_THREAD);

    task_executor executor;
    executor.run();

    auto future = executor.execute(taskflow);
    future.get();

    executor.stop();

    CHECK(num == 30);
}
} // namespace violet::test