#include "d3d12_descriptor_heap.hpp"
#include "d3d12_context.hpp"

namespace ash::graphics::d3d12
{
d3d12_descriptor_heap::d3d12_descriptor_heap(
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    std::size_t size,
    std::size_t increment_size,
    D3D12_DESCRIPTOR_HEAP_FLAGS flag)
    : m_increment_size(static_cast<UINT>(increment_size))
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = static_cast<UINT>(size);
    desc.Type = type;
    desc.Flags = flag;
    desc.NodeMask = 0;
    throw_if_failed(d3d12_context::device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));
}

std::size_t d3d12_descriptor_heap::allocate(std::size_t size)
{
    return m_index_allocator.allocate(size);
}

void d3d12_descriptor_heap::deallocate(std::size_t begin, std::size_t size)
{
    m_index_allocator.deallocate(begin, size);
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_descriptor_heap::cpu_handle(std::size_t index) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_heap->GetCPUDescriptorHandleForHeapStart(),
        static_cast<UINT>(index),
        m_increment_size);
}

D3D12_GPU_DESCRIPTOR_HANDLE d3d12_descriptor_heap::gpu_handle(std::size_t index) const
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_heap->GetGPUDescriptorHandleForHeapStart(),
        static_cast<UINT>(index),
        m_increment_size);
}
} // namespace ash::graphics::d3d12