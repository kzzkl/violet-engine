#include "d3d12_renderer.hpp"
#include "d3d12_command.hpp"
#include "d3d12_context.hpp"
#include "d3d12_utility.hpp"

namespace ash::graphics::d3d12
{
d3d12_swap_chain::d3d12_swap_chain(
    HWND handle,
    std::uint32_t width,
    std::uint32_t height,
    std::size_t multiple_sampling)
    : m_back_buffer_counter(0)
{
    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width = width;
    swap_chain_desc.Height = height;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.Format = RENDER_TARGET_FORMAT;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    // Multisampling a back buffer is not supported in D3D12.
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;

    throw_if_failed(d3d12_context::factory()->CreateSwapChainForHwnd(
        d3d12_context::command()->command_queue(),
        handle,
        &swap_chain_desc,
        nullptr,
        nullptr,
        &m_swap_chain));

    reset_back_buffers();
}

d3d12_swap_chain::~d3d12_swap_chain()
{
    m_back_buffers.clear();
    m_swap_chain = nullptr;
}

void d3d12_swap_chain::resize(std::uint32_t width, std::uint32_t height)
{
    m_back_buffers.clear();
    resize_buffer(width, height);
    reset_back_buffers();
}

void d3d12_swap_chain::present()
{
    throw_if_failed(m_swap_chain->Present(0, 0));
    ++m_back_buffer_counter;
}

void d3d12_swap_chain::resize_buffer(std::uint32_t width, std::uint32_t height)
{
    throw_if_failed(m_swap_chain->ResizeBuffers(
        2,
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        RENDER_TARGET_FORMAT,
        0));
    m_back_buffer_counter = 0;
}

void d3d12_swap_chain::reset_back_buffers()
{
    for (UINT i = 0; i < 2; ++i)
    {
        d3d12_ptr<D3D12Resource> buffer;
        m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&buffer));
        m_back_buffers.push_back(std::make_unique<d3d12_back_buffer>(buffer));
    }
}

std::size_t d3d12_swap_chain::back_buffer_index() const noexcept
{
    // m_back_buffer_counter % 2
    return m_back_buffer_counter & 1;
}

d3d12_renderer::d3d12_renderer(const renderer_desc& desc)
{
    d3d12_context::initialize(desc);

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
        adapter.Reset();
    }
}

d3d12_renderer::~d3d12_renderer()
{
    d3d12_context::shutdown();
}

void d3d12_renderer::present()
{
    d3d12_context::present();
}

render_command_interface* d3d12_renderer::allocate_command()
{
    d3d12_render_command* command = d3d12_context::command()->allocate_render_command();

    D3D12GraphicsCommandList* command_list = command->get();

    D3D12DescriptorHeap* heaps[] = {
        d3d12_context::resource()->visible_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->heap()};
    command_list->SetDescriptorHeaps(1, heaps);

    return command;
}

void d3d12_renderer::execute(render_command_interface* command)
{
    d3d12_render_command* c = static_cast<d3d12_render_command*>(command);
    d3d12_context::command()->execute_command(c);
}

resource_interface* d3d12_renderer::back_buffer()
{
    return d3d12_context::swap_chain().render_target();
}

void d3d12_renderer::resize(std::uint32_t width, std::uint32_t height)
{
    d3d12_context::command()->flush();
    d3d12_context::swap_chain().resize(width, height);
}
} // namespace ash::graphics::d3d12