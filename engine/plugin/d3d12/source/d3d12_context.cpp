#include "d3d12_context.hpp"

using namespace Microsoft::WRL;

namespace ash::graphics::d3d12
{
d3d12_context::d3d12_context()
{
}

d3d12_context& d3d12_context::instance()
{
    static d3d12_context instance;
    return instance;
}

bool d3d12_context::initialize(const context_config& config)
{
    UINT flag = 0;

#ifndef NDEBUG
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
        {
            debugController->EnableDebugLayer();
            flag |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    // Create DSGI factory
    HRESULT result = CreateDXGIFactory2(flag, IID_PPV_ARGS(m_factory.GetAddressOf()));
    throw_if_failed(result);

    // Create hardware device
    result = D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(m_device.GetAddressOf()));

    // If creating a hardware device fails, create a wrap device.
    if (FAILED(result))
    {
        ComPtr<DXGIAdapter> wrap_adapter;

        throw_if_failed(m_factory->EnumWarpAdapter(IID_PPV_ARGS(wrap_adapter.GetAddressOf())));
        throw_if_failed(D3D12CreateDevice(
            wrap_adapter.Get(),
            D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(m_device.GetAddressOf())));
    }

    m_diagnotor = std::make_unique<d3d12_diagnotor>();
    m_diagnotor->initialize(config);

    m_command = std::make_unique<d3d12_command_manager>();
    m_command->initialize();

    m_resource = std::make_unique<d3d12_resource_manager>();
    m_resource->initialize();

    m_renderer = std::make_unique<d3d12_renderer>();

    HWND hwnd = *static_cast<const HWND*>(config.handle);
    m_renderer->initialize(hwnd, config.width, config.height);

    m_command->get_command_list()->Close();

    D3D12CommandList* lists[] = {m_command->get_command_list()};
    m_command->get_command_queue()->ExecuteCommandLists(1, lists);
    m_command->flush();

    return true;
}
} // namespace ash::graphics::d3d12