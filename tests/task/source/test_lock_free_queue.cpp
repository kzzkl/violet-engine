#include "lock_free_queue.hpp"
#include "test_common.hpp"
#include <set>
#include <thread>

using namespace ash::task;
using namespace ash::test;

TEST_CASE("Concurrent allocate and deallocate of nodes", "[lock_free_node_pool][lock_free_queue]")
{
    struct node : public lock_free_node_pool<node>::node_type
    {
        int node_data;
    };

    using pool = lock_free_node_pool<node>;
    pool c;

    std::vector<std::thread> threads(NUM_THREAD);
    std::vector<pool::node_handle> data[NUM_THREAD];

    for (std::size_t i = 0; i < NUM_THREAD; ++i)
    {
        threads[i] = std::thread([&c, &data, i]() {
            std::size_t begin = i * NUM_DATA_PER_THREAD + 1;
            std::size_t end = begin + NUM_DATA_PER_THREAD;
            while (begin != end)
            {
                auto node = c.allocate();
                if (node->node_data != 0)
                {
                    data[i].push_back(node.get_pointer());
                }
                else
                {
                    node->node_data = static_cast<int>(begin);
                    c.deallocate(node);
                    ++begin;
                }
            }
        });
    }

    for (auto& t : threads)
        t.join();

    std::set<int> s;

    auto node = c.allocate();
    s.insert(node->node_data);
    c.deallocate(node);

    for (auto& v : data)
    {
        for (auto d : v)
        {
            s.insert(d->node_data);
            c.deallocate(d);
        }
    }

    CHECK(s.size() == NUM_THREAD * NUM_DATA_PER_THREAD);
}

TEST_CASE("lock free queue", "[lock_free_queue]")
{
    using queue = lock_free_queue<std::size_t>;
    queue q;

    std::vector<std::thread> threads(NUM_THREAD);
    for (std::size_t i = 0; i < NUM_THREAD; ++i)
    {
        threads[i] = std::thread([&q, i]() {
            std::size_t begin = i * NUM_DATA_PER_THREAD;
            std::size_t end = begin + NUM_DATA_PER_THREAD;

            for (std::size_t i = begin; i != end; ++i)
            {
                q.push(i);

                std::size_t t;
                if (q.pop(t))
                    q.push(t);
            }
        });
    }

    for (auto& t : threads)
        t.join();

    std::set<std::size_t> s;
    std::size_t t;
    while (q.pop(t))
        s.insert(t);

    CHECK(s.size() == NUM_THREAD * NUM_DATA_PER_THREAD);
}