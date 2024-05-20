#pragma once

#include "core/engine_module.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "graphics/render_interface.hpp"

namespace violet
{
class camera;
class rhi_plugin;
class graphics_module : public engine_module
{
public:
    graphics_module();
    virtual ~graphics_module();

    virtual bool initialize(const dictionary& config) override;
    virtual void shutdown() override;

    render_device* get_device() const noexcept { return m_device.get(); }

private:
    void begin_frame();
    void end_frame();
    void render();

    void update_light();
    void render_camera(camera* camera, rhi_semaphore* finish_semaphore);

    void switch_frame_resource();
    rhi_semaphore* allocate_semaphore();

    std::unique_ptr<render_device> m_device;
    std::unique_ptr<rhi_plugin> m_plugin;

    rhi_ptr<rhi_parameter> m_light;

    std::vector<std::vector<rhi_semaphore*>> m_used_semaphores;
    std::vector<rhi_semaphore*> m_free_semaphores;
    std::vector<rhi_ptr<rhi_semaphore>> m_semaphores;
};
} // namespace violet