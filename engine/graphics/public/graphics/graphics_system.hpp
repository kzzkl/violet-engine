#pragma once

#include "core/engine_system.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class camera;
class rhi_plugin;
class graphics_system : public engine_system
{
public:
    graphics_system();
    virtual ~graphics_system();

    bool initialize(const dictionary& config) override;
    void shutdown() override;

private:
    void begin_frame();
    void end_frame();
    void render();

    void update_light();
    rhi_semaphore* render(camera* camera);

    void switch_frame_resource();
    rhi_semaphore* allocate_semaphore();

    std::unique_ptr<rhi_plugin> m_plugin;

    std::vector<std::vector<rhi_semaphore*>> m_used_semaphores;
    std::vector<rhi_semaphore*> m_free_semaphores;
    std::vector<rhi_ptr<rhi_semaphore>> m_semaphores;

    std::unique_ptr<rdg_allocator> m_allocator;
    std::unique_ptr<render_context> m_context;

    std::uint32_t m_system_version{0};
};
} // namespace violet