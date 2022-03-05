#pragma once

#include "d3d12_common.hpp"

namespace ash::graphics::d3d12
{
class d3d12_command_manager
{
public:
    d3d12_command_manager();

    bool initialize();

    D3D12CommandQueue* get_command_queue() { return m_queue.Get(); }
    D3D12GraphicsCommandList* get_command_list() { return m_command_list.Get(); }

    D3D12CommandAllocator* get_command_allocator() { return m_allocator.Get(); }

    void flush();

private:
    Microsoft::WRL::ComPtr<D3D12CommandQueue> m_queue;
    Microsoft::WRL::ComPtr<D3D12CommandAllocator> m_allocator;

    Microsoft::WRL::ComPtr<D3D12GraphicsCommandList> m_command_list;

    UINT64 m_fence_counter;
    Microsoft::WRL::ComPtr<D3D12Fence> m_fence;
};
} // namespace ash::graphics::d3d12