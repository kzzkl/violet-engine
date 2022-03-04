#pragma once

#include "d3d12_common.hpp"
#include "d3d12_diagnotor.hpp"
#include <memory>

namespace ash::graphics::d3d12
{
class d3d12_context
{
public:
    static d3d12_context& instance();

    bool initialize();

    DXGIFactory* get_factory() { return m_factory.Get(); }
    D3D12Device* get_device() { return m_device.Get(); }

    d3d12_diagnotor* get_diagnotor() { return m_diagnotor.get(); }

private:
    d3d12_context();

    Microsoft::WRL::ComPtr<DXGIFactory> m_factory;
    Microsoft::WRL::ComPtr<D3D12Device> m_device;

    std::unique_ptr<d3d12_diagnotor> m_diagnotor;
};
} // namespace ash::graphics::d3d12