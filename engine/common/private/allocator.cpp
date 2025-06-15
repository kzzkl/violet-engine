#include "common/allocator.hpp"

namespace violet
{
buddy_allocator::buddy_allocator(std::uint32_t level)
    : m_level(level)
{
    std::uint32_t size = 1 << level;

    std::uint32_t node_count = size * 2 - 1;
    m_nodes.resize(node_count);
}

std::uint32_t buddy_allocator::allocate(std::uint32_t size)
{
    assert(size > 0);

    size = next_power_of_two(size);

    std::uint32_t length = 1 << m_level;

    if (size > length)
    {
        return no_space;
    }

    std::uint32_t index = 0;
    std::uint32_t level = 0;

    while (true)
    {
        if (size == length)
        {
            if (m_nodes[index] == NODE_STATE_UNUSED)
            {
                m_nodes[index] = NODE_STATE_USED;
                mark_parent(index);

                return get_offset(index, level);
            }
        }
        else
        {
            switch (m_nodes[index])
            {
            case NODE_STATE_USED:
            case NODE_STATE_FULL: {
                break;
            }
            case NODE_STATE_UNUSED: {
                m_nodes[index] = NODE_STATE_SPLIT;
                m_nodes[index * 2 + 1] = NODE_STATE_UNUSED;
                m_nodes[index * 2 + 2] = NODE_STATE_UNUSED;
                index = index * 2 + 1;
                length /= 2;
                level++;
                continue;
            }
            default: {
                index = index * 2 + 1;
                length /= 2;
                level++;
                continue;
            }
            }
        }

        if (index & 1)
        {
            ++index;
            continue;
        }
        while (true)
        {
            if (index == 0)
            {
                return no_space;
            }
            level--;
            length *= 2;
            index = get_parent(index);
            if (index & 1)
            {
                ++index;
                break;
            }
        }
    }

    return no_space;
}

void buddy_allocator::free(std::uint32_t offset)
{
    assert(offset < get_capacity());
    std::uint32_t left = 0;
    std::uint32_t length = 1 << m_level;
    std::uint32_t index = 0;

    while (true)
    {
        switch (m_nodes[index])
        {
        case NODE_STATE_USED: {
            assert(offset == left);
            combine(index);
            return;
        }
        case NODE_STATE_UNUSED: {
            assert(0);
            return;
        }
        default: {
            length /= 2;
            if (offset < left + length)
            {
                index = index * 2 + 1;
            }
            else
            {
                left += length;
                index = index * 2 + 2;
            }
            break;
        }
        }
    }
}

std::uint32_t buddy_allocator::get_size(std::uint32_t offset)
{
    assert(offset < get_capacity());
    std::uint32_t left = 0;
    std::uint32_t length = 1 << m_level;
    std::uint32_t index = 0;

    while (true)
    {
        switch (m_nodes[index])
        {
        case NODE_STATE_USED: {
            assert(offset == left);
            return length;
        }
        case NODE_STATE_UNUSED: {
            assert(0);
            return length;
        }
        default: {
            length /= 2;
            if (offset < left + length)
            {
                index = index * 2 + 1;
            }
            else
            {
                left += length;
                index = index * 2 + 2;
            }
            break;
        }
        }
    }
}

void buddy_allocator::mark_parent(std::uint32_t index)
{
    while (index != 0)
    {
        std::uint32_t buddy = get_buddy(index);
        if (m_nodes[buddy] == NODE_STATE_USED || m_nodes[buddy] == NODE_STATE_FULL)
        {
            index = get_parent(index);
            m_nodes[index] = NODE_STATE_FULL;
        }
        else
        {
            return;
        }
    }
}

void buddy_allocator::combine(std::uint32_t index)
{
    while (index != 0)
    {
        std::uint32_t buddy = get_buddy(index);
        if (m_nodes[buddy] != NODE_STATE_UNUSED)
        {
            m_nodes[index] = NODE_STATE_UNUSED;
            while (index != 0)
            {
                index = get_parent(index);
                if (m_nodes[index] == NODE_STATE_FULL)
                {
                    m_nodes[index] = NODE_STATE_SPLIT;
                }
            }
            return;
        }
        index = get_parent(index);
    }

    m_nodes[index] = NODE_STATE_UNUSED;
}
} // namespace violet