#pragma once

#include <atomic>
#include <mutex>

namespace ash::task
{
/**
 * @brief A pool that caches lock-free queue nodes
 *
 * When the lock-free queue is popped up, the memory occupied by the node will not be returned to
 * the OS but put into the pool, and it will be taken out of the pool first when applying for a node
 * next time.
 *
 * @tparam T node_type
 */
template <typename T>
class lock_free_queue_pool
{
public:
    using node_type = T;

public:
    lock_free_queue_pool() : m_free(nullptr) {}
    ~lock_free_queue_pool() { clear(); }

    /**
     * @brief Take out the node from the pool. When there is no node in the pool, a new node will be
     * created through the new operator.
     *
     * @return node_type
     */
    node_type* allocate()
    {
        // TODO: There is a bug if there is no lock here, so the current version is not a lock-free
        // queue
        std::lock_guard<std::mutex> lg(m_lock);

        node_type* old_head = m_free.load(std::memory_order_seq_cst);

        while (old_head &&
               !m_free.compare_exchange_weak(old_head, old_head->next, std::memory_order_seq_cst))
            ;

        return old_head ? old_head : new node_type{};
    }

    /**
     * @brief Add a node to the pool
     *
     * @param node
     */
    void free(node_type* node)
    {
        node->next = m_free.load(std::memory_order_seq_cst);
        while (!m_free.compare_exchange_weak(node->next, node, std::memory_order_seq_cst))
            ;
    }

    /**
     * @brief Clear all nodes cached in the pool
     */
    void clear()
    {
        while (true)
        {
            node_type* head = m_free.load();
            while (head && !m_free.compare_exchange_weak(head, head->next))
                ;

            if (head)
                delete head;
            else
                break;
        }
    }

private:
    std::mutex m_lock;
    std::atomic<node_type*> m_free;
};

template <typename T>
class lock_free_queue
{
public:
    using value_type = T;

public:
    lock_free_queue() { m_head = m_tail = m_pool.allocate(); }

    bool pop(value_type& value)
    {
        node* old_head = m_head.load();

        if (old_head == m_tail.load())
            return false;

        while (old_head && !m_head.compare_exchange_weak(old_head, old_head->next))
            ;

        value = std::move(old_head->data);
        m_pool.free(old_head);

        return true;
    }

    void push(const value_type& data)
    {
        node* new_tail = m_pool.allocate();
        node* old_tail = m_tail.load();

        while (!m_tail.compare_exchange_weak(old_tail, new_tail))
            ;

        old_tail->data = data;
        old_tail->next = new_tail;
    }

    bool empty() const { return m_head == m_tail; }

private:
    struct node
    {
        value_type data;
        node* next;
    };

    lock_free_queue_pool<node> m_pool;

    std::atomic<node*> m_head;
    std::atomic<node*> m_tail;
};
} // namespace ash::task