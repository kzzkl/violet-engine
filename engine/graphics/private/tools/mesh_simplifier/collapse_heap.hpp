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
        std::push_heap(m_heap.begin(), m_heap.end(), element_greater());
    }

    void pop()
    {
        std::pop_heap(m_heap.begin(), m_heap.end(), element_greater());
        m_heap.pop_back();
    }

    void erase(std::uint32_t edge)
    {
        auto iter = std::find_if(
            m_heap.begin(),
            m_heap.end(),
            [edge](const element& e)
            {
                return e.edge == edge;
            });

        if (iter != m_heap.end())
        {
            std::swap(*iter, m_heap.back());
            m_heap.pop_back();
            std::make_heap(m_heap.begin(), m_heap.end(), element_greater());
        }
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

    std::vector<element> m_heap;
};
} // namespace violet