#pragma once

#include <atomic>
#include <cstdint>
#include <limits>

namespace violet
{
template <typename T>
class tagged_pointer_compression
{
public:
    using value_type = T;

    using tag_type = std::uint16_t;
    using address_type = std::uint64_t;

public:
    tagged_pointer_compression() : m_address(0) {}
    tagged_pointer_compression(value_type* p) { m_address = reinterpret_cast<address_type>(p); }
    tagged_pointer_compression(value_type* p, tag_type tag)
    {
        m_address = reinterpret_cast<address_type>(p);
        m_tag[TAG_INDEX] = tag;
    }

    tag_type next_tag() const
    {
        tag_type next = (m_tag[TAG_INDEX] + 1) & std::numeric_limits<tag_type>::max();
        return next;
    }
    tag_type tag() const { return m_tag[TAG_INDEX]; }

    void pointer(value_type* p)
    {
        tag_type tag = m_tag[TAG_INDEX];
        m_address = reinterpret_cast<address_type>(p);
        m_tag[TAG_INDEX] = tag;
    }

    value_type* pointer() const
    {
        address_type address = m_address & ADDRESS_MASK;
        return reinterpret_cast<value_type*>(address);
    }

    value_type* operator->() { return pointer(); }

    bool operator==(const tagged_pointer_compression& p) const { return m_address == p.m_address; }
    bool operator!=(const tagged_pointer_compression& p) const { return !operator==(p); }

private:
    static constexpr address_type ADDRESS_MASK = 0xFFFFFFFFFFFFUL;
    static constexpr int TAG_INDEX = 3;

    union {
        tag_type m_tag[4];
        address_type m_address;
    };
};

template <typename T>
using tagged_pointer = tagged_pointer_compression<T>;

template <typename T1, typename T2>
tagged_pointer<T1> tagged_pointer_cast(tagged_pointer<T2> p)
{
    tagged_pointer<T1> result(static_cast<T1*>(p.pointer()), p.tag());
    return result;
}

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
    void free(node_handle node)
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
    using node_handle = typename pool_type::node_handle;

public:
    lock_free_queue() { m_head = m_tail = m_pool.allocate(); }
    ~lock_free_queue()
    {
        value_type temp;
        while (pop(temp))
        {
        }
    }

    /**
     * @brief Push the object into the queue.
     *
     * The push operation is thread-safe. Internally, it will try to connect the node to the end of
     * the queue in a loop.
     *
     * @param value object
     */
    void push(const value_type& value)
    {
        node_handle node = m_pool.allocate();
        node->data = value;
        node->next = nullptr;
        insert_node(node);
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
                        m_pool.free(old_head);
                        return true;
                    }
                }
            }
        }
    }

private:
    void insert_node(node_handle node)
    {
        while (true)
        {
            node_handle old_tail = m_tail.load();
            node_handle old_tail_next = old_tail->next.load();

            if (old_tail == m_tail.load())
            {
                if (old_tail_next == nullptr)
                {
                    node_handle new_tail_next(node.pointer(), old_tail_next.next_tag());
                    if (old_tail->next.compare_exchange_weak(old_tail_next, new_tail_next))
                    {
                        node_handle new_tail(node.pointer(), old_tail.next_tag());
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

    pool_type m_pool;

    std::atomic<node_handle> m_head;
    std::atomic<node_handle> m_tail;
};
} // namespace violet