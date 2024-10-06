#include "offsetAllocator.hpp"
#include <cstddef>
#include <cstdint>

namespace violet
{
using allocation = OffsetAllocator::Allocation;

class allocator
{
public:
    allocator(std::uint32_t size)
        : m_allocator(size)
    {
    }

    allocation allocate(std::uint32_t size)
    {
        return m_allocator.allocate(size);
    }

    void free(allocation allocation)
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
} // namespace violet
