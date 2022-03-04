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

bool d3d12_context::initialize()
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
    m_diagnotor->initialize();

    return true;
}
} // namespace ash::graphics::d3d12