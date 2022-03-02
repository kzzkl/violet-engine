#include "task_manager.hpp"
#include "test_common.hpp"
#include <iostream>

using namespace ash::task;
using namespace ash::test;

TEST_CASE("test_task_manager", "[task_manager]")
{
    task_manager manager(1);

    std::vector<int> data(30);

    auto task1 = manager.schedule("task 1", [&data]() {
        for (std::size_t i = 0; i < 10; ++i)
            data[i] = 1;
    });

    auto task2 = manager.schedule("task 2", [&data]() {
        for (std::size_t i = 10; i < 20; ++i)
            data[i] = data[i - 10] + 1;
    });
    task2->add_dependency(*task1);

    auto task3 = manager.schedule("task 3", [&data]() {
        for (std::size_t i = 20; i < 30; ++i)
            data[i] = data[i - 20] + 2;
    });
    task3->add_dependency(*task1);

    std::thread t([&manager]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        manager.stop();
    });
    manager.run(task1);

    t.join();

    for (std::size_t i = 0; i < 3; ++i)
    {
        for (std::size_t j = 0; j < 10; ++j)
        {
            REQUIRE(data[i * 10 + j] == i + 1);
        }
    }
}