#include "d3d12_renderer.hpp"
#include "d3d12_context.hpp"
#include "d3d12_frame_resource.hpp"
#include "d3d12_utility.hpp"
#include <iostream>

namespace ash::graphics::d3d12
{
d3d12_renderer::d3d12_renderer(
    HWND handle,
    UINT width,
    UINT height,
    D3D12GraphicsCommandList* command_list)
{
    auto factory = d3d12_context::factory();
    auto device = d3d12_context::device();

    // Get adapter information.
    d3d12_ptr<DXGIAdapter> adapter;
    for (UINT i = 0;; ++i)
    {
        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        m_adapter_info.push_back(wstring_to_string(desc.Description));
        adapter.Reset();
    }

    // Query sample level.
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS sample_level = {};
    sample_level.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    sample_level.Format = m_format;
    sample_level.NumQualityLevels = 0;
    sample_level.SampleCount = 4;
    throw_if_failed(d3d12_context::device()->CheckFeatureSupport(
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

    throw_if_failed(d3d12_context::factory()->CreateSwapChainForHwnd(
        d3d12_context::command()->get_command_queue(),
        handle,
        &swap_chain_desc,
        nullptr,
        nullptr,
        &m_swap_chain));

    for (UINT i = 0; i < 2; ++i)
    {
        d3d12_ptr<D3D12Resource> buffer;
        m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&buffer));
        m_back_buffer.push_back(std::make_unique<d3d12_back_buffer>(buffer));
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
        std::make_unique<d3d12_depth_stencil_buffer>(depth_stencil_desc, heap_properties, clear);
    m_depth_stencil_buffer->transition_state(D3D12_RESOURCE_STATE_DEPTH_WRITE, command_list);

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
}

void d3d12_renderer::begin_frame()
{
    d3d12_context::begin_frame();
}

void d3d12_renderer::end_frame()
{
    d3d12_context::end_frame();
}

render_command* d3d12_renderer::allocate_command()
{
    d3d12_render_command* command =
        d3d12_context::command()->allocate_render_command(d3d12_render_command_type::RENDER);

    D3D12GraphicsCommandList* command_list = command->get();

    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);

    auto back_buffer_handle = m_back_buffer[get_index()]->get_cpu_handle();
    auto depth_stencil_buffer_handle = m_depth_stencil_buffer->get_cpu_handle();
    command_list->OMSetRenderTargets(1, &back_buffer_handle, true, &depth_stencil_buffer_handle);

    return command;
}

void d3d12_renderer::execute(render_command* command)
{
    d3d12_render_command* c = static_cast<d3d12_render_command*>(command);
    d3d12_context::command()->execute_command(c);
}

resource* d3d12_renderer::get_back_buffer()
{
    return m_back_buffer[get_index()].get();
}

std::size_t d3d12_renderer::get_adapter_info(adapter_info* infos, std::size_t size) const
{
    std::size_t i = 0;
    for (; i < size && i < m_adapter_info.size(); ++i)
    {
        memcpy(infos[i].description, m_adapter_info[i].c_str(), m_adapter_info[i].size());
    }

    return i;
}

void d3d12_renderer::begin_frame(D3D12GraphicsCommandList* command_list)
{
    /*auto descriptor = d3d12_context::instance().get_descriptor();
    D3D12DescriptorHeap* heaps[] = {descriptor->get_cbv_heap()->get_heap(),
                                    descriptor->get_cbv_visible_heap()->get_heap()};*/

    m_back_buffer[get_index()]->transition_state(D3D12_RESOURCE_STATE_RENDER_TARGET, command_list);
    // command_list->SetDescriptorHeaps(1, heaps);
    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);

    static const float clear_color[] = {0.0f, 0.0f, 0.5f, 1.0f};
    command_list->ClearRenderTargetView(
        m_back_buffer[get_index()]->get_cpu_handle(),
        clear_color,
        1,
        &m_scissor_rect);
    command_list->ClearDepthStencilView(
        m_depth_stencil_buffer->get_cpu_handle(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f,
        0,
        0,
        nullptr);
}

void d3d12_renderer::end_frame(D3D12GraphicsCommandList* command_list)
{
    m_back_buffer[get_index()]->transition_state(D3D12_RESOURCE_STATE_PRESENT, command_list);
}

void d3d12_renderer::present()
{
    throw_if_failed(m_swap_chain->Present(0, 0));
}

/*void d3d12_renderer::bind(D3D12GraphicsCommandList* command_list)
{
    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);

    auto back_buffer_handle = m_back_buffer[get_index()].get_cpu_handle();
    auto depth_stencil_buffer_handle = m_depth_stencil_buffer.get_cpu_handle();
    command_list->OMSetRenderTargets(1, &back_buffer_handle, true, &depth_stencil_buffer_handle);
}*/

UINT64 d3d12_renderer::get_index() const
{
    return d3d12_frame_counter::frame_counter() % 2;
}
} // namespace ash::graphics::d3d12