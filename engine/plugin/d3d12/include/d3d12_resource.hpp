#pragma once

#include "d3d12_common.hpp"
#include <array>

namespace ash::graphics::d3d12
{
class d3d12_resource
{
public:
    d3d12_resource() = default;
    d3d12_resource(
        Microsoft::WRL::ComPtr<D3D12Resource> resource,
        D3D12_CPU_DESCRIPTOR_HANDLE view,
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);

    D3D12Resource* get_resource() const { return m_resource.Get(); }
    const D3D12_CPU_DESCRIPTOR_HANDLE& get_view() const { return m_view; }

    void transition_state(D3D12_RESOURCE_STATES state, D3D12GraphicsCommandList* command_list);

private:
    Microsoft::WRL::ComPtr<D3D12Resource> m_resource;
    D3D12_CPU_DESCRIPTOR_HANDLE m_view;

    D3D12_RESOURCE_STATES m_state;
};

class d3d12_resource_manager
{
public:
    d3d12_resource_manager();

    bool initialize();

    D3D12_CPU_DESCRIPTOR_HANDLE get_render_target_view(UINT index);
    D3D12_CPU_DESCRIPTOR_HANDLE get_depth_stencil_view();

    d3d12_resource make_back_buffer(Microsoft::WRL::ComPtr<D3D12Resource> buffer);
    d3d12_resource make_depth_stencil(
        const D3D12_RESOURCE_DESC& desc,
        const CD3DX12_HEAP_PROPERTIES& heap_properties,
        const D3D12_CLEAR_VALUE& clear);

private:
    Microsoft::WRL::ComPtr<D3D12DescriptorHeap> m_rtv_heap;
    Microsoft::WRL::ComPtr<D3D12DescriptorHeap> m_dsv_heap;

    UINT m_rtv_counter;

    std::array<UINT, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_descriptor_size;
};
} // namespace ash::graphics::d3d12