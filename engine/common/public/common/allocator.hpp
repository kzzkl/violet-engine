#pragma once

#include "offsetAllocator.hpp"
#include <cassert>
#include <cstdint>
#include <vector>

namespace violet
{
using buffer_allocation = OffsetAllocator::Allocation;

class buffer_allocator
{
public:
    buffer_allocator(std::uint32_t size)
        : m_allocator(size)
    {
    }

    buffer_allocation allocate(std::uint32_t size)
    {
        buffer_allocation allocation = m_allocator.allocate(size);
        assert(allocation.offset != buffer_allocation::NO_SPACE);
        return allocation;
    }

    void free(buffer_allocation allocation)
    {
        m_allocator.free(allocation);
    }

    void reset()
    {
        m_allocator.reset();
    }

private:
    OffsetAllocator::Allocator m_allocator;
};

template <typename T>
class index_allocator
{
public:
    using index_type = T;

    index_allocator(index_type max_count = 0xffffffff)
        : m_max_count(max_count)
    {
    }

    index_type allocate()
    {
        if (!m_free.empty())
        {
            index_type index = m_free.back();
            m_free.pop_back();
            return index;
        }

        return m_index_count < m_max_count ? m_index_count++ : 0xffffffff;
    }

    void free(index_type index)
    {
        m_free.push_back(index);
    }

    void reset()
    {
        m_free.clear();
        m_index_count = 0;
    }

    std::size_t get_size() const noexcept
    {
        return m_index_count - m_free.size();
    }

private:
    std::vector<index_type> m_free;
    index_type m_index_count{0};

    index_type m_max_count;
};
} // namespace violet
