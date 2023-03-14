#pragma once

#include "tagged_pointer.hpp"
#include <atomic>

namespace violet::core
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
class lock_free_node_pool
{
public:
    using node_handle = tagged_pointer<T>;
    using value_type = T;

    struct node_type
    {
        node_type() : next(nullptr) {}
        virtual ~node_type() = default;

        tagged_pointer<node_type> next;
    };

private:
    using node_pointer = tagged_pointer<node_type>;

public:
    lock_free_node_pool() : m_free(nullptr) {}
    ~lock_free_node_pool() { clear(); }

    /**
     * @brief Take out the node from the pool. When there is no node in the pool, a new node will be
     * created through the new operator.
     *
     * @return handle
     */
    node_handle allocate()
    {
        node_pointer old_head = m_free.load(std::memory_order_consume);
        while (true)
        {
            if (!old_head.pointer())
                return new value_type();

            node_type* new_head_ptr = old_head->next.pointer();
            node_pointer new_head(new_head_ptr, old_head.next_tag());

            if (m_free.compare_exchange_weak(old_head, new_head))
            {
                return handle(old_head);
            }
        }
    }

    /**
     * @brief Add a node to the pool
     *
     * @param node
     */
    void deallocate(node_handle node)
    {
        node_pointer old_head = m_free.load(std::memory_order_consume);
        node_type* new_head_ptr = pointer(node).pointer();

        while (true)
        {
            node_pointer new_head(new_head_ptr, old_head.tag());
            new_head->next.pointer(old_head.pointer());

            if (m_free.compare_exchange_weak(old_head, new_head))
                return;
        }
    }

    /**
     * @brief Clear the node in the cache and return the memory occupied by the node to the OS
     */
    void clear()
    {
        node_pointer head = m_free.load();
        while (head.pointer())
        {
            node_pointer next = head->next;
            delete head.pointer();
            head = next;
        }
        m_free.store(node_pointer(nullptr));
    }

private:
    node_handle handle(node_pointer pointer) const
    {
        return tagged_pointer_cast<value_type>(pointer);
    }

    node_pointer pointer(node_handle handle) const
    {
        return tagged_pointer_cast<node_type>(handle);
    }

    std::atomic<node_pointer> m_free;
};

template <typename T>
class lock_free_queue
{
public:
    using value_type = T;

private:
    struct node_type : public lock_free_node_pool<node_type>::node_type
    {
        node_type() : data{}, next(nullptr) {}

        value_type data;
        std::atomic<typename lock_free_node_pool<node_type>::node_handle> next;
    };

    using pool_type = lock_free_node_pool<node_type>;
    using node_handle = pool_type::node_handle;

public:
    lock_free_queue() { m_head = m_tail = m_pool.allocate(); }

    /**
     * @brief Push the object into the queue.
     *
     * The push operation is thread-safe. Internally, it will try to connect the node to the end of
     * the queue in a loop.
     *
     * @param data object
     */
    void push(const value_type& data)
    {
        node_handle new_node = m_pool.allocate();
        new_node->data = data;
        new_node->next = nullptr;

        while (true)
        {
            node_handle old_tail = m_tail.load();
            node_handle old_tail_next = old_tail->next.load();

            if (old_tail == m_tail.load())
            {
                if (old_tail_next == nullptr)
                {
                    node_handle new_tail_next(new_node.pointer(), old_tail_next.next_tag());
                    if (old_tail->next.compare_exchange_weak(old_tail_next, new_tail_next))
                    {
                        node_handle new_tail(new_node.pointer(), old_tail.next_tag());
                        m_tail.compare_exchange_strong(old_tail, new_tail);
                        return;
                    }
                }
                else
                {
                    node_handle new_tail(old_tail_next.pointer(), old_tail.next_tag());
                    m_tail.compare_exchange_strong(old_tail, new_tail);
                }
            }
        }
    }

    /**
     * @brief Assigns the object at the head of the queue to the parameter and pops the head of the
     * queue.
     *
     * Note that the value of the parameter may also be changed when the pop fails.
     *
     * @param value Parameter to store the head element.
     * @return Returns false if the queue is empty, otherwise returns true.
     */
    bool pop(value_type& value)
    {
        while (true)
        {
            node_handle old_head = m_head.load();
            node_handle old_head_next = old_head->next.load();
            node_handle old_tail = m_tail.load();

            if (old_head == m_head.load())
            {
                if (old_head.pointer() == old_tail.pointer())
                {
                    if (old_head_next.pointer() == nullptr)
                    {
                        return false;
                    }
                    else
                    {
                        node_handle new_tail(old_head_next.pointer(), old_tail.next_tag());
                        m_tail.compare_exchange_strong(old_tail, new_tail);
                    }
                }
                else
                {
                    if (old_head_next.pointer() == nullptr)
                        continue;

                    value = old_head_next->data;

                    node_handle new_head(old_head_next.pointer(), old_head.next_tag());
                    if (m_head.compare_exchange_weak(old_head, new_head))
                    {
                        m_pool.deallocate(old_head);
                        return true;
                    }
                }
            }
        }
    }

    /**
     * @brief Returns whether the queue is empty.
     *
     * This operation is only guaranteed to be correct if no other thread is performing push or pop.
     *
     * @return Is the queue empty
     */
    bool empty() const { return m_head.load() == m_tail.load(); }

private:
    pool_type m_pool;

    std::atomic<node_handle> m_head;
    std::atomic<node_handle> m_tail;
};
} // namespace violet::core