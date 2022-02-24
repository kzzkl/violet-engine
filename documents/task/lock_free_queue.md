#

### TODO

这段代码多线程下会导致 m_free 出现环，node == node->next

```c++
class atomic_counter
{
public:
    struct node_type
    {
        node_type* next;
        node_type() :next(nullptr) {}
    };

public:
    atomic_counter() : m_free(nullptr) {
    }

    node_type* allocate()
    {
        node_type* old_head = m_free.load();
        while (old_head &&
               !m_free.compare_exchange_weak(old_head, old_head->next))
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
```