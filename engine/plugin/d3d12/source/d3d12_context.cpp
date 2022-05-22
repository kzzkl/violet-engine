#include "d3d12_context.hpp"
#include "d3d12_command.hpp"
#include "d3d12_pipeline.hpp"
#include "d3d12_renderer.hpp"
#include "d3d12_resource.hpp"
#include <DirectXColors.h>

using namespace DirectX;

namespace ash::graphics::d3d12
{
d3d12_context::d3d12_context() noexcept
{
}

d3d12_context& d3d12_context::instance() noexcept
{
    static d3d12_context instance;
    return instance;
}

bool d3d12_context::on_initialize(const renderer_desc& desc)
{
    d3d12_frame_counter::initialize(0, desc.frame_resource);

    UINT flag = 0;

#ifndef NDEBUG
    {
        d3d12_ptr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            flag |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    // Create DSGI factory
    HRESULT result = CreateDXGIFactory2(flag, IID_PPV_ARGS(&m_factory));
    throw_if_failed(result);

    // Create hardware device
    result = D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device));

    // If creating a hardware device fails, create a wrap device.
    if (FAILED(result))
    {
        d3d12_ptr<DXGIAdapter> wrap_adapter;

        throw_if_failed(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&wrap_adapter)));
        throw_if_failed(D3D12CreateDevice(
            wrap_adapter.Get(),
            D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)));
    }

    m_command = std::make_unique<d3d12_command_queue>(desc.render_concurrency);
    m_resource = std::make_unique<d3d12_resource_manager>();
    m_frame_buffer_manager = std::make_unique<d3d12_frame_buffer_manager>();
    m_swap_chain = std::make_unique<d3d12_swap_chain>(
        static_cast<HWND>(desc.window_handle),
        desc.width,
        desc.height);

    return true;
}

void d3d12_context::on_shutdown()
{
    m_command->flush();

    m_command = nullptr;
    m_swap_chain = nullptr;
    m_frame_buffer_manager = nullptr;
    m_resource = nullptr;

    m_factory = nullptr;
    m_device = nullptr;
}

void d3d12_context::on_begin_frame()
{
}

void d3d12_context::on_end_frame()
{
    m_command->execute_batch();
    m_swap_chain->present();

    d3d12_frame_counter::tick();

    m_command->switch_frame_resources();
    m_resource->switch_frame_resources();
}
} // namespace ash::graphics::d3d12