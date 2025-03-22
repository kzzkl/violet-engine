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

    disjoint_set(std::size_t size)
        : m_parents(size),
          m_ranks(size)
    {
        std::iota(m_parents.begin(), m_parents.end(), 0);
    }

    void merge(value_type a, value_type b)
    {
        std::size_t root_a = find(a);
        std::size_t root_b = find(b);

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

    std::size_t find(value_type value)
    {
        // Find root.
        std::size_t root = m_parents[value];
        while (root != m_parents[root])
        {
            root = m_parents[root];
        }

        // Path compression.
        std::size_t current = value;
        while (m_parents[current] != root)
        {
            m_parents[current] = root;
            current = m_parents[current];
        }

        return root;
    }

    std::size_t get_size() const noexcept
    {
        return m_parents.size();
    }

private:
    std::vector<std::size_t> m_parents;
    std::vector<std::size_t> m_ranks;
};
} // namespace violet