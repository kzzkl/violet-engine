#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

namespace violet
{
class collapse_heap
{
public:
    struct element
    {
        std::uint32_t edge;
        float error;
    };

    void push(const element& element)
    {
        m_heap.push_back(element);
        std::ranges::push_heap(m_heap, element_greater());
    }

    void pop()
    {
        std::ranges::pop_heap(m_heap, element_greater());
        m_heap.pop_back();
    }

    bool erase(std::uint32_t edge)
    {
        if (update(edge, std::numeric_limits<float>::lowest()))
        {
            pop();
            return true;
        }

        return false;
    }

    bool update(std::uint32_t edge, float error)
    {
        auto iter = std::ranges::find_if(
            m_heap,
            [edge](const element& e)
            {
                return e.edge == edge;
            });

        if (iter == m_heap.end())
        {
            return false;
        }

        if (iter->error > error)
        {
            iter->error = error;
            sift_up(std::distance(m_heap.begin(), iter));
        }
        else
        {
            iter->error = error;
            sift_down(std::distance(m_heap.begin(), iter));
        }

        return true;
    }

    const element& top() const
    {
        return m_heap.front();
    }

    bool empty() const noexcept
    {
        return m_heap.empty();
    }

    std::size_t size() const noexcept
    {
        return m_heap.size();
    }

private:
    struct element_greater
    {
        bool operator()(const element& a, const element& b) const noexcept
        {
            return a.error > b.error;
        }
    };

    void sift_up(std::size_t index)
    {
        while (index > 0)
        {
            std::size_t parent = (index - 1) / 2;

            if (m_heap[index].error < m_heap[parent].error)
            {
                std::swap(m_heap[index], m_heap[parent]);
                index = parent;
            }
            else
            {
                break;
            }
        }
    }

    void sift_down(std::size_t index)
    {
        std::size_t size = m_heap.size();

        std::size_t child = (index * 2) + 1;
        while (child < size)
        {
            if (child + 1 < size && m_heap[child].error > m_heap[child + 1].error)
            {
                ++child;
            }

            if (m_heap[index].error > m_heap[child].error)
            {
                std::swap(m_heap[index], m_heap[child]);
                index = child;
                child = index * 2 + 1;
            }
            else
            {
                break;
            }
        }
    }

    std::vector<element> m_heap;
};
} // namespace violet