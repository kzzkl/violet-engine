#pragma once

#include "core/engine.hpp"
#include "graphics/render_scene.hpp"
#include "render_graph/rdg_allocator.hpp"

namespace violet
{
class camera_component;
class rhi_plugin;
class render_scene_manager;
class graphics_system : public system
{
public:
    graphics_system();
    virtual ~graphics_system();

    void install(application& app) override;
    bool initialize(const dictionary& config) override;

private:
    void begin_frame();
    void end_frame();
    void render();

    rhi_fence* render(
        const camera_component* camera,
        rhi_parameter* camera_parameter,
        const render_scene& scene);

    void switch_frame_resource();
    rhi_fence* allocate_fence();

    std::unique_ptr<rhi_plugin> m_plugin;

    std::unique_ptr<render_scene_manager> m_scene_manager;

    std::vector<std::vector<rhi_fence*>> m_used_fences;
    std::vector<rhi_fence*> m_free_fences;
    std::vector<rhi_ptr<rhi_fence>> m_fences;

    std::unique_ptr<rdg_allocator> m_allocator;

    rhi_ptr<rhi_fence> m_frame_fence;
    std::uint64_t m_frame_fence_value{0};
    std::vector<std::uint64_t> m_frame_fence_values;

    rhi_ptr<rhi_fence> m_update_fence;
    std::uint64_t m_update_fence_value{0};

    std::uint32_t m_system_version{0};
};
} // namespace violet