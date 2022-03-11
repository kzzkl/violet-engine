#pragma once

#include "d3d12_command.hpp"
#include "d3d12_common.hpp"
#include "d3d12_renderer.hpp"
#include "d3d12_resource.hpp"
#include <memory>

namespace ash::graphics::d3d12
{
class d3d12_context
{
public:
    static bool initialize(const context_config& config)
    {
        return instance().do_initialize(config);
    }

    static void begin_frame() { instance().do_begin_frame(); }
    static void end_frame() { instance().do_end_frame(); }

    static DXGIFactory* factory() { return instance().m_factory.Get(); }
    static D3D12Device* device() { return instance().m_device.Get(); }

    static d3d12_command_manager* command() { return instance().m_command.get(); }
    static d3d12_renderer* renderer() { return instance().m_renderer.get(); }
    static d3d12_resource_manager* resource() { return instance().m_resource.get(); }

private:
    d3d12_context();
    static d3d12_context& instance();

    bool do_initialize(const context_config& config);
    void do_begin_frame();
    void do_end_frame();

    d3d12_ptr<DXGIFactory> m_factory;
    d3d12_ptr<D3D12Device> m_device;

    std::unique_ptr<d3d12_command_manager> m_command;
    std::unique_ptr<d3d12_renderer> m_renderer;
    std::unique_ptr<d3d12_resource_manager> m_resource;
};
} // namespace ash::graphics::d3d12