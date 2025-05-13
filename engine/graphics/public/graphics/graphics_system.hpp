#pragma once

#include "core/engine.hpp"
#include "graphics/render_device.hpp"
#include "render_graph/rdg_allocator.hpp"

#ifndef NDEBUG
#include "graphics/debug_drawer.hpp"
#endif

namespace violet
{
class camera_component;
class camera_component_meta;
class rhi_plugin;
class render_scene_manager;
class gpu_buffer_uploader;
class graphics_system : public system
{
public:
    graphics_system();
    virtual ~graphics_system();

    void install(application& app) override;
    bool initialize(const dictionary& config) override;

#ifndef NDEBUG
    debug_drawer& get_debug_drawer()
    {
        return *m_debug_drawer;
    }
#endif

private:
    struct render_context
    {
        const camera_component* camera;
        const camera_component_meta* camera_meta;
        std::uint32_t layer;
    };

    void begin_frame();
    void end_frame();
    void render();

    rhi_fence* render(const render_context& context);

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

    std::unique_ptr<gpu_buffer_uploader> m_gpu_buffer_uploader;

    std::uint32_t m_system_version{0};

#ifndef NDEBUG
    std::unique_ptr<debug_drawer> m_debug_drawer;
#endif
};
} // namespace violet