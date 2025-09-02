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
    static constexpr std::uint32_t no_space = OffsetAllocator::Allocation::NO_SPACE;

    buffer_allocator(std::size_t size)
        : m_allocator(static_cast<std::uint32_t>(size))
    {
    }

    buffer_allocation allocate(std::size_t size)
    {
        buffer_allocation allocation = m_allocator.allocate(static_cast<std::uint32_t>(size));
        assert(allocation.offset != no_space);
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

class index_allocator
{
public:
    static constexpr std::uint32_t no_space = 0xFFFFFFFF;

    index_allocator(std::uint32_t offset = 0, std::uint32_t capacity = 0xFFFFFFFF)
        : m_offset(offset),
          m_capacity(capacity)
    {
    }

    std::uint32_t allocate()
    {
        if (!m_free.empty())
        {
            std::uint32_t index = m_free.back();
            m_free.pop_back();
            return index;
        }

        return m_count < m_capacity ? m_offset + m_count++ : no_space;
    }

    void free(std::uint32_t index)
    {
        m_free.push_back(index);
    }

    void reset()
    {
        m_free.clear();
        m_count = 0;
    }

    std::size_t get_size() const noexcept
    {
        return m_count - m_free.size();
    }

private:
    std::vector<std::uint32_t> m_free;
    std::uint32_t m_offset;
    std::uint32_t m_count{0};
    std::uint32_t m_capacity;
};

class buddy_allocator
{
public:
    static constexpr std::uint32_t no_space = 0xFFFFFFFF;

    buddy_allocator(std::uint32_t level);

    std::uint32_t allocate(std::uint32_t size);

    void free(std::uint32_t offset);

    std::uint32_t get_size(std::uint32_t offset);

    std::uint32_t get_capacity() const noexcept
    {
        return 1 << m_level;
    }

private:
    enum node_state : std::uint8_t
    {
        NODE_STATE_UNUSED,
        NODE_STATE_USED,
        NODE_STATE_SPLIT,
        NODE_STATE_FULL,
    };

    void mark_parent(std::uint32_t index);

    void combine(std::uint32_t index);

    std::uint32_t get_parent(std::uint32_t index) const noexcept
    {
        assert(index != 0);
        return ((index + 1) / 2) - 1;
    }

    std::uint32_t get_buddy(std::uint32_t index) const noexcept
    {
        assert(index != 0);
        return index - 1 + ((index & 1) * 2);
    }

    std::uint32_t get_offset(std::uint32_t index, std::uint32_t level) const noexcept
    {
        return ((index + 1) - (1 << level)) << (m_level - level);
    }

    std::uint32_t m_level;
    std::vector<node_state> m_nodes;
};
} // namespace violet
