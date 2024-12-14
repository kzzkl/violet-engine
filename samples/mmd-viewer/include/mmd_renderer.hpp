#pragma once

#include "graphics/renderer.hpp"

namespace violet::sample
{
class mmd_renderer : public renderer
{
public:
    void render(render_graph& graph, const render_scene& scene, const render_camera& camera)
        override;

private:
    void add_skinning_pass(render_graph& graph);
    void add_present_pass(render_graph& graph, const render_camera& camera);

    rhi_texture_extent m_render_extent;
    rdg_texture* m_render_target{nullptr};
    rdg_texture* m_depth_buffer{nullptr};
};
} // namespace violet::sample