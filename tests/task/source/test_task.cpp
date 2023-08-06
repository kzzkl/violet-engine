#include "core/task/task.hpp"
#include "core/task/task_executor.hpp"
#include "test_common.hpp"
#include <queue>

namespace violet::test
{
TEST_CASE("dependencies between tasks", "[task]")
{
    int num = 0;

    task_graph<int, int> graph;
    task<std::tuple<int&>(int, int)>& t1 = graph.then(
        [&num](int a, int b)
        {
            num = a + b;
            return task_base::resolve(std::ref(num));
        });

    task<void(int&)>& t2 = t1.then([](int& num) { num += 2; });
    task<void()>& t3 = t2.then([&num]() { num /= 2; });

    task_executor executor;
    executor.run();

    auto future = executor.execute(graph, 4, 6);
    future.get();

    executor.stop();

    CHECK(num == 6);
}
} // namespace violet::test