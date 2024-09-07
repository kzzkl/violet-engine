#include "task/task.hpp"
#include "task/task_executor.hpp"
#include "task/task_graph_printer.hpp"
#include "test_common.hpp"
#include <iostream>
#include <queue>

namespace violet::test
{
TEST_CASE("Dependencies between tasks", "[task]")
{
    int num = 0;
    std::cout << "main thread: " << std::this_thread::get_id() << std::endl;

    task_graph graph;

    task& task_1 = graph.add_task()
                       .set_name("task 1")
                       .set_execute(
                           [&]()
                           {
                               std::cout << "task 1: " << std::this_thread::get_id() << std::endl;
                               num = 10;
                           })
                       .set_options(TASK_OPTION_MAIN_THREAD);

    task& task_2 = graph.add_task()
                       .set_name("task 2")
                       .set_execute(
                           [&]()
                           {
                               std::cout << "task 2: " << std::this_thread::get_id() << std::endl;
                               CHECK(num == 10);
                               num = 20;
                           })
                       .add_dependency(task_1);

    task& task_3 = graph.add_task()
                       .set_name("task 3")
                       .set_execute(
                           [&]()
                           {
                               std::cout << "task 3: " << std::this_thread::get_id() << std::endl;
                               CHECK(num == 20);
                               num = 30;
                           })
                       .add_dependency(task_2)
                       .set_options(TASK_OPTION_MAIN_THREAD);

    task_executor executor;
    executor.run();

    auto future = executor.execute(graph);
    future.get();

    executor.stop();

    CHECK(num == 30);
}

TEST_CASE("Task group", "[task]")
{
    task_graph graph;

    task_group& group_1 = graph.add_group().set_name("Group 1");

    task& task_1 = graph.add_task()
                       .set_name("task 1")
                       .set_execute(
                           []()
                           {
                               std::cout << "task 1" << std::endl;
                           })
                       .set_group(group_1);

    task_group& group_2 = graph.add_group().set_name("Group 2");
    group_2.add_dependency(group_1);
}

TEST_CASE("Task dependency", "[task]")
{
    task_graph graph;

    task& task_a = graph.add_task().set_name("A");
    task& task_b = graph.add_task().set_name("B");
    task& task_d = graph.add_task().set_name("D");
    task& task_c = graph.add_task().set_name("C");
    task& task_e = graph.add_task().set_name("E");
    task& task_f = graph.add_task().set_name("F");
    task& task_g = graph.add_task().set_name("G");
    task& task_h = graph.add_task().set_name("H");
    task& task_i = graph.add_task().set_name("I");
    task& task_j = graph.add_task().set_name("J");
    task& task_k = graph.add_task().set_name("K");
    task& task_l = graph.add_task().set_name("L");

    task_group& group_a = graph.add_group().set_name("Group A");
    task_group& group_b = graph.add_group().set_name("Group B");
    task_group& group_c = graph.add_group().set_name("Group C");
    task_group& group_d = graph.add_group().set_name("Group D");

    task_a.set_group(group_a);
    task_b.set_group(group_a).add_dependency(task_a);
    task_c.set_group(group_a);
    task_d.set_group(group_b);
    task_e.add_dependency(group_a);
    task_f.add_dependency(group_b);
    task_g.add_dependency(task_f);
    task_h.set_group(group_c);
    task_i.set_group(group_c).add_dependency(task_f);
    task_j.set_group(group_d);
    task_k.set_group(group_d);

    group_b.add_dependency(task_c);
    group_c.add_dependency(task_e);
    group_d.set_group(group_c).add_dependency(task_h);

    graph.reset();

    task_graph_printer::print(graph);
}
} // namespace violet::test