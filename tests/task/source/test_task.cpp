#include "task/task.hpp"
#include "task/task_executor.hpp"
#include "test_common.hpp"
#include <queue>

namespace violet::test
{
TEST_CASE("dependencies between tasks", "[task]")
{
    int num = 0;

    task_graph<int, int> graph;
    task<int&>& t1 = graph.get_root().then(
        [&num](int a, int b)
        {
            num = a + b;
            return std::make_tuple(std::ref(num));
        });

    task<>& t2 = t1.then(
        [](int& num)
        {
            num += 2;
        });
    task<>& t3 = t2.then(
        [&num]()
        {
            num /= 2;
        });

    task_executor executor;
    executor.run();

    auto future = executor.execute(graph, 4, 6);
    future.get();

    executor.stop();

    CHECK(num == 6);
}
} // namespace violet::test