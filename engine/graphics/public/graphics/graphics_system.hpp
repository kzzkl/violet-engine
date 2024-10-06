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

    void add_parameter();
    void remove_parameter();
    void update_parameter();

    void update_light();
    rhi_fence* render(const camera* camera, rhi_parameter* camera_parameter);

    void switch_frame_resource();
    rhi_fence* allocate_fence();

    std::unique_ptr<rhi_plugin> m_plugin;

    std::vector<std::vector<rhi_fence*>> m_used_fences;
    std::vector<rhi_fence*> m_free_fences;
    std::vector<rhi_ptr<rhi_fence>> m_fences;

    std::unique_ptr<rdg_allocator> m_allocator;
    std::unique_ptr<render_context> m_context;

    rhi_ptr<rhi_fence> m_frame_fence;
    std::uint64_t m_frame_fence_value{0};
    std::vector<std::uint64_t> m_frame_fence_values{0};

    std::uint32_t m_system_version{0};
};
} // namespace violet