#include "log.hpp"
#include "task_manager.hpp"
#include <crtdbg.h>
#include <functional>
#include <iostream>
#include <set>
#include <stdlib.h>
#include <thread>

using namespace violet::task;
using namespace violet::common;

struct test_class
{
    int data;
};

bool test_queue()
{
    constexpr std::size_t NUM_THREAD = 32;
    constexpr std::size_t NUM_DATA_PER_THREAD = 100000;

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
    std::vector<std::size_t> v;
    std::size_t t;
    while (q.pop(t))
    {
        s.insert(t);
        v.push_back(t);
    }

    return s.size() == NUM_THREAD * NUM_DATA_PER_THREAD;
}

bool test_pool()
{
    constexpr std::size_t NUM_THREAD = 4;
    constexpr std::size_t NUM_DATA_PER_THREAD = 1000;

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

    std::size_t a = c.size();
    c.clear();
    std::size_t b = c.size();

    return s.size() == NUM_THREAD * NUM_DATA_PER_THREAD;
}

using namespace violet::task;

void test_task()
{
    task_manager manager(8);

    auto task1 = manager.schedule("task 1", []() {
        log::debug("thread[{}]: task 1: hello", std::this_thread::get_id());
    });
    task1->add_dependency(*manager.get_root());

    auto task2 = manager.schedule("task 2", []() {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        log::debug("thread[{}]: task 2: hello", std::this_thread::get_id());
    });
    task2->add_dependency(*manager.get_root());

    auto task3 = manager.schedule("task 3", []() {
        log::debug("thread[{}]: task 3: hello", std::this_thread::get_id());
    });
    task3->add_dependency(*task1);

    auto task4 = manager.schedule("task 4", []() {
        log::debug("thread[{}]: task 4: hello", std::this_thread::get_id());
    });
    task4->add_dependency(*task3);
    task4->add_dependency(*task2);

    auto task5 = manager.schedule("task 5", []() {
        log::debug("thread[{}]: task 5: hello", std::this_thread::get_id());
    });
    task5->add_dependency(*task4);

    manager.get_root()->add_dependency(*task5);

    manager.run();

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(10));

    manager.stop();
}

int main()
{
    // bool result = test_pool();
    // bool result = test_queue();

    // std::cout << result << std::endl;

    test_task();

    return 0;
}