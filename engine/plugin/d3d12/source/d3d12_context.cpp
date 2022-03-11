#include "d3d12_context.hpp"
#include <DirectXColors.h>
using namespace DirectX;

namespace ash::graphics::d3d12
{
d3d12_context::d3d12_context()
{
    d3d12_frame_counter::initialize(0, 3);
}

d3d12_context& d3d12_context::instance()
{
    static d3d12_context instance;
    return instance;
}

bool d3d12_context::do_initialize(const context_config& config)
{
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

    m_command = std::make_unique<d3d12_command_manager>(config.render_concurrency);
    m_resource = std::make_unique<d3d12_resource_manager>();

    auto command_list = m_command->allocate_dynamic_command();
    m_renderer = std::make_unique<d3d12_renderer>(
        *static_cast<const HWND*>(config.window_handle),
        config.width,
        config.height,
        command_list.get());

    m_command->execute_command(command_list);

    return true;
}

void d3d12_context::do_begin_frame()
{
    d3d12_render_command* pre_command =
        m_command->allocate_render_command(d3d12_render_command_type::PRE_RENDER);
    m_renderer->begin_frame(pre_command->get());
    m_command->execute_command(pre_command);
}

void d3d12_context::do_end_frame()
{
    d3d12_render_command* post_command =
        m_command->allocate_render_command(d3d12_render_command_type::POST_RENDER);
    m_renderer->end_frame(post_command->get());
    m_command->execute_command(post_command);

    m_command->execute_batch();
    m_renderer->present();

    d3d12_frame_counter::tick();

    m_command->switch_frame_resources();
    m_resource->switch_frame_resources();
}
} // namespace ash::graphics::d3d12