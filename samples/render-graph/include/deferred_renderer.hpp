#pragma once

#include "graphics/renderer.hpp"

namespace violet::sample
{
class deferred_renderer : public renderer
{
public:
    deferred_renderer();

    virtual void render(
        render_graph& graph,
        const render_context& context,
        const render_camera& camera);

private:
    void add_mesh_pass(
        render_graph& graph,
        const render_context& context,
        const render_camera& camera);
    void add_skybox_pass(
        render_graph& graph,
        const render_camera& camera);

    rdg_texture* m_render_target;
    rdg_texture* m_depth_buffer;
};
} // namespace violet::sample