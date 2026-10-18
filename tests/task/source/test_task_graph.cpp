#include "task/task_graph.hpp"
#include "test_task_common.hpp"
#include <atomic>
#include <cstdint>
#include <stdexcept>
#include <thread>

namespace violet::test
{
TEST_CASE("task_graph: a single task executes exactly once", "[task2][graph]")
{
    scheduler_scope scope(3);
    task_graph graph;

    std::atomic<std::int32_t> executed{0};

    graph.add_task().set_name("only").set_execute(
        [&]()
        {
            executed.fetch_add(1);
        });

    graph.execute_sync(scope.get());

    CHECK(executed.load() == 1);
}

TEST_CASE("task_graph: a chain runs in dependency order", "[task2][graph]")
{
    scheduler_scope scope(3);
    task_graph graph;

    std::atomic<std::int32_t> next{0};
    std::atomic<std::int32_t> a_order{-1};
    std::atomic<std::int32_t> b_order{-1};
    std::atomic<std::int32_t> c_order{-1};

    task_node& a = graph.add_task().set_name("a");
    a.set_execute(
        [&]()
        {
            a_order.store(next.fetch_add(1));
        });

    task_node& b = graph.add_task().set_name("b").add_dependency(a);
    b.set_execute(
        [&]()
        {
            b_order.store(next.fetch_add(1));
        });

    task_node& c = graph.add_task().set_name("c").add_dependency(b);
    c.set_execute(
        [&]()
        {
            c_order.store(next.fetch_add(1));
        });

    graph.execute_sync(scope.get());

    CHECK(a_order.load() == 0);
    CHECK(b_order.load() == 1);
    CHECK(c_order.load() == 2);
}

TEST_CASE("task_graph: a task waits for every predecessor", "[task2][graph]")
{
    scheduler_scope scope(3);
    task_graph graph;

    std::atomic<std::int32_t> finished{0};
    std::atomic<std::int32_t> join_seen{-1};
    std::atomic<std::int32_t> join_count{0};

    task_node* predecessors[3] = {};

    for (std::int32_t i = 0; i < 3; ++i)
    {
        predecessors[i] = &graph.add_task().set_execute(
            [&]()
            {
                finished.fetch_add(1);
            });
    }

    task_node& join =
        graph.add_task().add_dependency(*predecessors[0], *predecessors[1], *predecessors[2]);

    join.set_execute(
        [&]()
        {
            join_seen.store(finished.load());
            join_count.fetch_add(1);
        });

    graph.execute_sync(scope.get());

    CHECK(join_seen.load() == 3);
    CHECK(join_count.load() == 1);
}

TEST_CASE("task_graph: transitive reduction keeps every node reachable", "[task2][graph]")
{
    // a -> b -> c and the redundant a -> c. After the transitive reduction only b -> c
    // is left, so the join count of c must come from the reduced edge set as well.
    // A mismatch here leaves c waiting forever.
    scheduler_scope scope(3);
    task_graph graph;

    std::atomic<std::int32_t> executed{0};

    task_node& a = graph.add_task().set_name("a").set_execute(
        [&]()
        {
            executed.fetch_add(1);
        });

    task_node& b = graph.add_task().set_name("b").add_dependency(a);
    b.set_execute(
        [&]()
        {
            executed.fetch_add(1);
        });

    task_node& c = graph.add_task().set_name("c").add_dependency(a, b);
    c.set_execute(
        [&]()
        {
            executed.fetch_add(1);
        });

    graph.execute_sync(scope.get());

    CHECK(executed.load() == 3);
}

TEST_CASE("task_graph: diamond fan-out and fan-in", "[task2][graph]")
{
    scheduler_scope scope(3);
    task_graph graph;

    std::atomic<std::int32_t> fork_done{0};
    std::atomic<std::int32_t> join_seen{-1};

    task_node& root = graph.add_task().set_name("root").set_execute(
        [&]()
        {
            fork_done.fetch_add(1);
        });

    task_node& left = graph.add_task().set_name("left").add_dependency(root);
    left.set_execute(
        [&]()
        {
            fork_done.fetch_add(1);
        });

    task_node& right = graph.add_task().set_name("right").add_dependency(root);
    right.set_execute(
        [&]()
        {
            fork_done.fetch_add(1);
        });

    task_node& join = graph.add_task().set_name("join").add_dependency(left, right);
    join.set_execute(
        [&]()
        {
            join_seen.store(fork_done.load());
        });

    graph.execute_sync(scope.get());

    CHECK(fork_done.load() == 3);
    CHECK(join_seen.load() == 3);
}

TEST_CASE("task_graph: groups order their members", "[task2][graph]")
{
    scheduler_scope scope(3);
    task_graph graph;

    std::atomic<std::int32_t> next{0};
    std::atomic<std::int32_t> update_order{-1};
    std::atomic<std::int32_t> post_order{-1};

    task_group& update = graph.add_group().set_name("Update");
    task_group& post = graph.add_group().set_name("Post").add_dependency(update);

    graph.add_task()
        .set_name("update work")
        .set_group(update)
        .set_execute(
            [&]()
            {
                update_order.store(next.fetch_add(1));
            });

    graph.add_task()
        .set_name("post work")
        .set_group(post)
        .set_execute(
            [&]()
            {
                post_order.store(next.fetch_add(1));
            });

    graph.execute_sync(scope.get());

    CHECK(update_order.load() == 0);
    CHECK(post_order.load() == 1);
}

TEST_CASE("task_graph: a main-affinity task runs on the main thread", "[task2][graph]")
{
    scheduler_scope scope(3);
    task_graph graph;

    const std::thread::id main_id = std::this_thread::get_id();
    std::atomic<bool> executed{false};
    std::atomic<bool> ran_on_main{false};

    graph.add_task()
        .set_name("main")
        .set_affinity(TASK_AFFINITY_MAIN)
        .set_execute(
            [&]()
            {
                ran_on_main.store(std::this_thread::get_id() == main_id);
                executed.store(true);
            });

    graph.execute_sync(scope.get());

    CHECK(executed.load());
    CHECK(ran_on_main.load());
}

TEST_CASE("task_graph: repeated frames leave no leftover state", "[task2][graph]")
{
    constexpr std::int32_t frame_count = 100;

    scheduler_scope scope(3);
    task_graph graph;

    std::atomic<std::int32_t> a_count{0};
    std::atomic<std::int32_t> b_count{0};

    task_node& a = graph.add_task().set_name("a").set_execute(
        [&]()
        {
            a_count.fetch_add(1);
        });

    task_node& b = graph.add_task().set_name("b").add_dependency(a).set_execute(
        [&]()
        {
            b_count.fetch_add(1);
        });

    for (std::int32_t frame = 0; frame < frame_count; ++frame)
    {
        graph.execute_sync(scope.get());
    }

    CHECK(a_count.load() == frame_count);
    CHECK(b_count.load() == frame_count);
}

TEST_CASE("task_graph: tasks added between frames take effect", "[task2][graph]")
{
    scheduler_scope scope(3);
    task_graph graph;

    std::atomic<std::int32_t> a_count{0};
    std::atomic<std::int32_t> c_count{0};

    task_node& a = graph.add_task().set_name("a").set_execute(
        [&]()
        {
            a_count.fetch_add(1);
        });

    graph.execute_sync(scope.get());
    CHECK(a_count.load() == 1);

    // add_dependency() marks the graph dirty, so the next execute() recompiles.
    task_node& c = graph.add_task().set_name("c").add_dependency(a);
    c.set_execute(
        [&]()
        {
            c_count.fetch_add(1);
        });

    graph.execute_sync(scope.get());

    CHECK(a_count.load() == 2);
    CHECK(c_count.load() == 1);
}

TEST_CASE("task_graph: an empty graph completes immediately", "[task2][graph]")
{
    scheduler_scope scope(2);
    task_graph graph;

    CHECK(graph.get_task_count() == 0);

    graph.execute_sync(scope.get()); // must return instead of waiting for a task

    CHECK(graph.get_task_count() == 0);
}

TEST_CASE("task_graph: node and group lookup", "[task2][graph]")
{
    task_graph graph;

    task_node& a = graph.add_task().set_name("alpha");
    task_group& group = graph.add_group().set_name("Beta");

    CHECK(graph.has_task("alpha"));
    CHECK_FALSE(graph.has_task("missing"));
    CHECK(&graph.get_task("alpha") == &a);

    REQUIRE_THROWS_AS(graph.get_task("missing"), std::runtime_error);

    CHECK(graph.has_group("Beta"));
    CHECK_FALSE(graph.has_group("missing"));
    CHECK(&graph.get_group("Beta") == &group);

    REQUIRE_THROWS_AS(graph.get_group("missing"), std::runtime_error);

    // The group itself contributes a begin and an end task.
    CHECK(graph.get_task_count() == 3u);
}

TEST_CASE("task_graph: printer walks a compiled graph", "[task2][graph]")
{
    scheduler_scope scope(3);
    task_graph graph;

    task_group& update = graph.add_group().set_name("Update");

    task_node& a = graph.add_task().set_name("a").set_group(update);
    task_node& b = graph.add_task().set_name("b").add_dependency(a);
    graph.add_task().set_name("c").add_dependency(b).set_affinity(TASK_AFFINITY_MAIN);

    graph.execute_sync(scope.get());

    graph.print(); // smoke test: must not crash on a compiled graph
}
} // namespace violet::test
