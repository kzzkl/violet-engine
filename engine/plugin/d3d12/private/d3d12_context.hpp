#pragma once

#include "d3d12_common.hpp"
#include <memory>

namespace violet::graphics::d3d12
{
class d3d12_command_queue;
class d3d12_swap_chain;
class d3d12_resource_manager;
class d3d12_cache;

class d3d12_context
{
public:
    static bool initialize(const rhi_desc& desc) { return instance().on_initialize(desc); }
    static void shutdown() { instance().on_shutdown(); }
    static void present() { instance().on_present(); }

    inline static DXGIFactory* factory() noexcept { return instance().m_factory.Get(); }
    inline static D3D12Device* device() noexcept { return instance().m_device.Get(); }

    inline static d3d12_command_queue* command() noexcept { return instance().m_command.get(); }
    inline static d3d12_swap_chain& swap_chain() noexcept { return *instance().m_swap_chain; }
    inline static d3d12_resource_manager* resource() noexcept
    {
        return instance().m_resource.get();
    }
    inline static d3d12_cache& cache() noexcept
    {
        return *instance().m_cache;
    }

private:
    d3d12_context() noexcept;
    static d3d12_context& instance() noexcept;

    bool on_initialize(const rhi_desc& desc);
    void on_shutdown();
    void on_present();

    d3d12_ptr<DXGIFactory> m_factory;
    d3d12_ptr<D3D12Device> m_device;

    std::unique_ptr<d3d12_command_queue> m_command;
    std::unique_ptr<d3d12_swap_chain> m_swap_chain;
    std::unique_ptr<d3d12_resource_manager> m_resource;
    std::unique_ptr<d3d12_cache> m_cache;
};
} // namespace violet::graphics::d3d12