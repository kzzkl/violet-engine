#pragma once

#include <numeric>
#include <vector>

namespace violet
{
template <typename T>
    requires std::is_integral_v<T>
class disjoint_set
{
public:
    using value_type = T;

    disjoint_set(value_type size)
        : m_parents(size),
          m_ranks(size)
    {
        std::iota(m_parents.begin(), m_parents.end(), 0);
    }

    void merge(value_type a, value_type b)
    {
        value_type root_a = find(a);
        value_type root_b = find(b);

        if (root_a == root_b)
        {
            return;
        }

        // Merge smaller tree to larger tree.
        if (m_ranks[root_a] < m_ranks[root_b])
        {
            m_parents[root_a] = root_b;
        }
        else if (m_ranks[root_a] > m_ranks[root_b])
        {
            m_parents[root_b] = root_a;
        }
        else
        {
            m_parents[root_b] = root_a;
            ++m_ranks[root_a];
        }
    }

    value_type find(value_type value)
    {
        // Find root.
        value_type root = m_parents[value];
        while (root != m_parents[root])
        {
            root = m_parents[root];
        }

        // Path compression.
        value_type current = value;
        while (m_parents[current] != root)
        {
            m_parents[current] = root;
            current = m_parents[current];
        }

        return root;
    }

    std::uint32_t get_size() const noexcept
    {
        return static_cast<std::uint32_t>(m_parents.size());
    }

private:
    std::vector<value_type> m_parents;
    std::vector<std::uint32_t> m_ranks;
};
} // namespace violet