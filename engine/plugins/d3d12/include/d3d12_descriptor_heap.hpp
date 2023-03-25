#pragma once

#include "d3d12_common.hpp"
#include "d3d12_frame_resource.hpp"
#include <array>
#include <deque>

namespace violet::d3d12
{
template <typename T>
class index_allocator
{
public:
    using value_type = T;

    struct index_range
    {
        value_type begin;
        value_type end;
    };

public:
    index_allocator() : m_next_index(0) {}

    value_type allocate(std::size_t size = 1)
    {
        for (auto iter = m_free.begin(); iter != m_free.end(); ++iter)
        {
            if (iter->end - iter->begin >= size)
            {
                value_type result = iter->begin;

                iter->begin += static_cast<value_type>(size);
                if (iter->begin == iter->end)
                    m_free.erase(iter);

                return result;
            }
        }

        value_type reuslt = m_next_index;
        m_next_index += static_cast<value_type>(size);

        return reuslt;
    }

    void deallocate(const value_type& begin, std::size_t size = 1)
    {
        value_type end = begin + size;
        for (index_range& free_range : m_free)
        {
            if (free_range.begin == end)
            {
                free_range.begin -= size;
                return;
            }
            else if (free_range.end == begin)
            {
                free_range.end += size;
                return;
            }
        }

        m_free.push_back(index_range{begin, end});
    }

private:
    std::deque<index_range> m_free;
    value_type m_next_index;
};

static constexpr std::size_t INVALID_DESCRIPTOR_INDEX = -1;

class d3d12_descriptor_heap
{
public:
    d3d12_descriptor_heap(
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        std::size_t size,
        std::size_t increment_size,
        D3D12_DESCRIPTOR_HEAP_FLAGS flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    std::size_t allocate(std::size_t size = 1);
    void deallocate(std::size_t begin, std::size_t size = 1);

    inline D3D12DescriptorHeap* heap() const noexcept { return m_heap.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle(std::size_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle(std::size_t index) const;

    inline UINT increment_size() const noexcept { return m_increment_size; }

private:
    index_allocator<std::size_t> m_index_allocator;
    d3d12_ptr<D3D12DescriptorHeap> m_heap;

    UINT m_increment_size;
};
} // namespace violet::d3d12