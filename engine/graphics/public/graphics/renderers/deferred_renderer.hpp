#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class deferred_renderer : public renderer
{
public:
    void render(render_graph& graph, const render_scene& scene, const render_camera& camera)
        override;

private:
    void add_cull_pass(render_graph& graph, const render_scene& scene, const render_camera& camera);
    void add_mesh_pass(render_graph& graph, const render_scene& scene, const render_camera& camera);
    void add_lighting_pass(render_graph& graph, const render_scene& scene);
    void add_present_pass(render_graph& graph, const render_camera& camera);

    rdg_texture* m_render_target{nullptr};

    rdg_texture* m_gbuffer_albedo{nullptr};
    rdg_texture* m_depth_buffer{nullptr};

    rdg_buffer* m_command_buffer{nullptr};
    rdg_buffer* m_count_buffer{nullptr};

    rhi_texture_extent m_render_extent;
};
} // namespace violet