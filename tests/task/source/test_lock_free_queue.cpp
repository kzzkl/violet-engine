#include "lock_free_queue.hpp"
#include "test_common.hpp"
#include <set>

using namespace ash::task;
using namespace ash::test;

TEST_CASE("lock free queue", "[lock_free_queue]")
{
    /*
    lock_free_queue<std::size_t> queue;

    std::vector<std::thread> threads(NUM_THREAD);
    for (std::size_t i = 0; i < NUM_THREAD; ++i)
    {
        threads[i] = std::thread([&queue, i]() {
            std::size_t begin = i * NUM_DATA_PER_THREAD;
            std::size_t end = begin + NUM_DATA_PER_THREAD;

            for (std::size_t i = begin; i != end; ++i)
            {
                queue.push(i);

                std::size_t t;
                if (queue.pop(t))
                    queue.push(t);
            }
        });
    }

    for (auto& t : threads)
        t.join();

    std::set<std::size_t> numbers;
    std::size_t temp;
    while (queue.pop(temp))
        numbers.insert(temp);

    temp = 0;
    for (std::size_t number : numbers)
    {
        REQUIRE(temp == number);
        ++temp;
    }
    */
}