#pragma once

#include "d3d12_command.hpp"
#include "d3d12_common.hpp"
#include "d3d12_diagnotor.hpp"
#include "d3d12_renderer.hpp"
#include "d3d12_resource.hpp"
#include <memory>

namespace ash::graphics::d3d12
{
class d3d12_context
{
public:
    using context_config = ash::graphics::external::graphics_context_config;

public:
    static d3d12_context& instance();

    bool initialize(const context_config& config);

    DXGIFactory* get_factory() { return m_factory.Get(); }
    D3D12Device* get_device() { return m_device.Get(); }

    d3d12_diagnotor* get_diagnotor() { return m_diagnotor.get(); }
    d3d12_command_manager* get_command() { return m_command.get(); }
    d3d12_renderer* get_renderer() { return m_renderer.get(); }
    d3d12_resource_manager* get_resource() { return m_resource.get(); }

private:
    d3d12_context();

    Microsoft::WRL::ComPtr<DXGIFactory> m_factory;
    Microsoft::WRL::ComPtr<D3D12Device> m_device;

    std::unique_ptr<d3d12_diagnotor> m_diagnotor;
    std::unique_ptr<d3d12_command_manager> m_command;
    std::unique_ptr<d3d12_renderer> m_renderer;
    std::unique_ptr<d3d12_resource_manager> m_resource;
};
} // namespace ash::graphics::d3d12