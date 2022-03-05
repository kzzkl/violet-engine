#include "d3d12_resource.hpp"
#include "d3d12_context.hpp"

using namespace Microsoft::WRL;

namespace ash::graphics::d3d12
{
d3d12_resource::d3d12_resource(
    Microsoft::WRL::ComPtr<D3D12Resource> resource,
    D3D12_CPU_DESCRIPTOR_HANDLE view,
    D3D12_RESOURCE_STATES state)
    : m_resource(resource),
      m_view(view),
      m_state(state)
{
}

void d3d12_resource::transition_state(
    D3D12_RESOURCE_STATES state,
    D3D12GraphicsCommandList* command_list)
{
    auto transition = CD3DX12_RESOURCE_BARRIER::Transition(m_resource.Get(), m_state, state);
    command_list->ResourceBarrier(1, &transition);
    m_state = state;
}

d3d12_resource_manager::d3d12_resource_manager() : m_rtv_counter(0)
{
}

bool d3d12_resource_manager::initialize()
{
    auto device = d3d12_context::instance().get_device();

    // Get the descriptor size.
    m_descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // Create descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC rtv_desc = {};
    rtv_desc.NumDescriptors = 2;
    rtv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtv_desc.NodeMask = 0;
    throw_if_failed(
        device->CreateDescriptorHeap(&rtv_desc, IID_PPV_ARGS(m_rtv_heap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsv_desc = {};
    dsv_desc.NumDescriptors = 1;
    dsv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsv_desc.NodeMask = 0;
    throw_if_failed(
        device->CreateDescriptorHeap(&dsv_desc, IID_PPV_ARGS(m_dsv_heap.GetAddressOf())));

    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_resource_manager::get_render_target_view(UINT index)
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_rtv_heap->GetCPUDescriptorHandleForHeapStart(),
        index,
        m_descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_resource_manager::get_depth_stencil_view()
{
    return m_dsv_heap->GetCPUDescriptorHandleForHeapStart();
}

d3d12_resource d3d12_resource_manager::make_back_buffer(
    Microsoft::WRL::ComPtr<D3D12Resource> buffer)
{
    d3d12_resource result(
        buffer,
        get_render_target_view(m_rtv_counter),
        D3D12_RESOURCE_STATE_PRESENT);

    auto device = d3d12_context::instance().get_device();
    device->CreateRenderTargetView(result.get_resource(), nullptr, result.get_view());

    ++m_rtv_counter;

    return result;
}

d3d12_resource d3d12_resource_manager::make_depth_stencil(
    const D3D12_RESOURCE_DESC& desc,
    const CD3DX12_HEAP_PROPERTIES& heap_properties,
    const D3D12_CLEAR_VALUE& clear)
{
    auto device = d3d12_context::instance().get_device();

    ComPtr<D3D12Resource> resource;
    throw_if_failed(device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        &clear,
        IID_PPV_ARGS(resource.GetAddressOf())));

    D3D12_CPU_DESCRIPTOR_HANDLE view = get_depth_stencil_view();
    device->CreateDepthStencilView(resource.Get(), nullptr, view);

    return d3d12_resource(resource, view, D3D12_RESOURCE_STATE_COMMON);
}
} // namespace ash::graphics::d3d12