#include "d3d12_renderer.hpp"
#include "d3d12_context.hpp"
#include <DirectXColors.h>
#include <iostream>

using namespace Microsoft::WRL;

namespace ash::graphics::d3d12
{
d3d12_renderer::d3d12_renderer() : m_frame_counter(0)
{
}

bool d3d12_renderer::initialize(HWND handle, UINT width, UINT height)
{
    auto factory = d3d12_context::instance().get_factory();
    auto device = d3d12_context::instance().get_device();
    auto resource = d3d12_context::instance().get_resource();
    auto command = d3d12_context::instance().get_command();

    // Query sample level.
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS sample_level = {};
    sample_level.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    sample_level.Format = m_format;
    sample_level.NumQualityLevels = 0;
    sample_level.SampleCount = 4;
    throw_if_failed(device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &sample_level,
        sizeof(sample_level)));

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width = width;
    swap_chain_desc.Height = height;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.Format = m_format;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    // Multisampling a back buffer is not supported in D3D12.
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;

    throw_if_failed(factory->CreateSwapChainForHwnd(
        d3d12_context::instance().get_command()->get_command_queue(),
        handle,
        &swap_chain_desc,
        nullptr,
        nullptr,
        m_swap_chain.GetAddressOf()));

    for (UINT i = 0; i < 2; ++i)
    {
        ComPtr<D3D12Resource> buffer;
        m_swap_chain->GetBuffer(i, IID_PPV_ARGS(buffer.GetAddressOf()));
        m_back_buffer.push_back(resource->make_back_buffer(buffer));
    }

    // Create a depth stencil buffer and view.
    D3D12_RESOURCE_DESC depth_stencil_desc = {};
    depth_stencil_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depth_stencil_desc.Alignment = 0;
    depth_stencil_desc.Width = width;
    depth_stencil_desc.Height = height;
    depth_stencil_desc.DepthOrArraySize = 1;
    depth_stencil_desc.MipLevels = 1;
    depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_desc.SampleDesc.Count = 1;
    depth_stencil_desc.SampleDesc.Quality = 0;
    depth_stencil_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depth_stencil_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clear.DepthStencil.Depth = 1.0f;
    clear.DepthStencil.Stencil = 0;

    m_depth_stencil_buffer =
        resource->make_depth_stencil(depth_stencil_desc, heap_properties, clear);

    m_depth_stencil_buffer.transition_state(
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        command->get_command_list());

    m_viewport.Width = static_cast<float>(width);
    m_viewport.Height = static_cast<float>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;

    m_scissor_rect.top = 0;
    m_scissor_rect.bottom = height;
    m_scissor_rect.left = 0;
    m_scissor_rect.right = width;

    return true;
}

void d3d12_renderer::begin_frame()
{
    auto command = d3d12_context::instance().get_command();

    auto command_allocator = command->get_command_allocator();
    auto command_list = command->get_command_list();

    throw_if_failed(command_allocator->Reset());
    throw_if_failed(command_list->Reset(command_allocator, nullptr));

    m_back_buffer[get_index()].transition_state(D3D12_RESOURCE_STATE_RENDER_TARGET, command_list);

    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);

    command_list->ClearRenderTargetView(
        m_back_buffer[get_index()].get_view(),
        DirectX::Colors::Aqua,
        1,
        &m_scissor_rect);
    command_list->ClearDepthStencilView(
        m_depth_stencil_buffer.get_view(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f,
        0,
        0,
        nullptr);

    command_list->OMSetRenderTargets(
        1,
        &m_back_buffer[get_index()].get_view(),
        true,
        &m_depth_stencil_buffer.get_view());

    m_back_buffer[get_index()].transition_state(D3D12_RESOURCE_STATE_PRESENT, command_list);
}

void d3d12_renderer::end_frame()
{
    auto command = d3d12_context::instance().get_command();
    auto command_list = command->get_command_list();

    throw_if_failed(command_list->Close());

    D3D12CommandList* lists[] = {command_list};
    command->get_command_queue()->ExecuteCommandLists(1, lists);

    throw_if_failed(m_swap_chain->Present(0, 0));

    command->flush();

    ++m_frame_counter;
}
} // namespace ash::graphics::d3d12