#include "log.hpp"
#include "task_manager.hpp"
#include <functional>
#include <iostream>
#include <set>
#include <thread>

using namespace ash::task;

struct test_class
{
    int data;
};

void test_pool()
{
    struct node
    {
        std::pair<std::size_t, int> data;
        node* next;
    };

    lock_free_queue_pool<node> p;

    const std::size_t DATA_SIZE = 160000;
    const std::size_t THREAD_NUM = 4;

    const std::size_t DATA_PER_THREAD = DATA_SIZE / THREAD_NUM;

    std::vector<std::thread> threads(THREAD_NUM);
    std::vector<int> target_data(DATA_SIZE);

    std::vector<node*> address[THREAD_NUM] = {};

    for (std::size_t i = 0; i < THREAD_NUM; ++i)
    {
        threads[i] = std::thread([&p, &target_data, &address, DATA_PER_THREAD, i]() {
            auto begin = i * DATA_PER_THREAD;
            auto end = begin + DATA_PER_THREAD;

            for (std::size_t j = begin; j != end; ++j)
            {
                node* n = p.allocate();
                if (n->data.first != 0)
                    target_data[n->data.first] = n->data.second;

                n->data.first = j;
                n->data.second = static_cast<int>(j);
                p.free(n);
            }
        });
    }

    for (auto& t : threads)
        t.join();

    for (std::size_t i = 0; i < DATA_SIZE; ++i)
    {
        node* n = p.allocate();
        target_data[n->data.first] = n->data.second;
    }

    for (std::size_t i = 0; i < target_data.size(); ++i)
    {
        if (target_data[i] != i)
        {
            std::cout << "no pass" << std::endl;
            break;
        }
    }
}

void test_queue()
{
    using queue = lock_free_queue<std::size_t>;
    queue q;

    const std::size_t DATA_SIZE = 3200000;
    const std::size_t THREAD_NUM = 32;

    const std::size_t DATA_PER_THREAD = DATA_SIZE / THREAD_NUM;

    std::vector<std::thread> threads(THREAD_NUM);
    for (std::size_t i = 0; i < THREAD_NUM; ++i)
    {
        threads[i] = std::thread([&q, DATA_PER_THREAD, i]() {
            std::size_t begin = i * DATA_PER_THREAD;
            std::size_t end = begin + DATA_PER_THREAD;

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
    {
        s.insert(t);
    }

    std::cout << std::endl;
}

void test_pool_free()
{
    struct node
    {
        std::pair<std::size_t, int> data;
        node* next;
    };

    lock_free_queue_pool<node> p;

    const std::size_t THREAD_NUM = 32;
    std::vector<std::thread> threads(THREAD_NUM);

    for (std::size_t i = 0; i < THREAD_NUM; ++i)
    {
        threads[i] = std::thread([&p]() {
            for (std::size_t j = 0; j < 10000; ++j)
            {
                node* n = new node();
                p.free(n);
            }
        });
    }

    for (auto& t : threads)
        t.join();
}

class atomic_counter
{
public:
    struct node_type
    {
        node_type* next;
        node_type() : next(nullptr) {}
    };

public:
    atomic_counter() : m_free(nullptr) {}

    node_type* allocate()
    {
        node_type* old_head = m_free.load();
        while (old_head && !m_free.compare_exchange_weak(old_head, old_head->next))
            ;
        return old_head ? old_head : new node_type();
    }

    void free(node_type* node)
    {
        node->next = m_free.load();
        while (!m_free.compare_exchange_weak(node->next, node))
            ;
    }

private:
    std::atomic<node_type*> m_free;
};

class atomic_counter_2
{
public:
    template <typename T>
    struct tag_pointer
    {
    public:
        tag_pointer() : m_data(0) {}
        tag_pointer(T* p) : m_tag(0), m_pointer(reinterpret_cast<uint64_t>(p)) {}

        void tag() { ++m_tag; }

        void set_pointer(T* p)
        {
            ++m_tag;
            m_pointer = reinterpret_cast<uint64_t>(p);
        }

        T* get_pointer() const { return reinterpret_cast<T*>(m_pointer); }

        bool operator==(const tag_pointer<T>& other) const { return m_data == other.m_data; }
        T* operator->() { return get_pointer(); }

        operator bool() const { return m_pointer != 0; }

    private:
        union {
            struct
            {
                uint64_t m_tag : 4;
                uint64_t m_pointer : 60;
            };
            uint64_t m_data;
        };
    };

    struct node_type
    {
        tag_pointer<node_type> next;
        node_type() : next(nullptr) {}
    };

public:
    atomic_counter_2() : m_free(nullptr) {}

    node_type* allocate()
    {
        tag_pointer<node_type> old_head = m_free.load();
        old_head.tag();
        do
        {
            old_head = m_free.load();
            if (old_head == nullptr)
                return new node_type();

        } while (!m_free.compare_exchange_weak(old_head, old_head->next));

        return old_head.get_pointer();
    }

    void free(node_type* node)
    {
        node->next = m_free.load();
        while (!m_free.compare_exchange_weak(node->next, node))
            ;
    }

private:
    std::atomic<tag_pointer<node_type>> m_free;
};

void test_counter()
{
    using pool = atomic_counter;
    using node_type = pool::node_type;
    pool c;

    const std::size_t THREAD_NUM = 3;

    std::vector<std::thread> threads(THREAD_NUM);
    std::vector<node_type*> data[THREAD_NUM];

    for (std::size_t i = 0; i < THREAD_NUM; ++i)
    {
        threads[i] = std::thread([&c, &data, i]() {
            for (std::size_t j = 0; j < 10000; ++j)
            {
                auto node = c.allocate();
                if (node->next == node)
                    throw;
                data[i].push_back(node);
                c.free(node);
            }
        });
    }

    for (auto& t : threads)
        t.join();

    std::set<node_type*> s;
    for (auto& v : data)
    {
        for (node_type* d : v)
        {
            s.insert(d);
        }
    }

    std::cout << std::endl;
}

using namespace ash::task;
using namespace ash::common;

int main()
{
    // test_pool();
    // test_queue();
    // test_pool_free();
    // test_counter();

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

        manager.run();

        std::this_thread::sleep_for(std::chrono::seconds(5));
        manager.stop();

        // std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    log::debug("end");

    return 0;
}