#pragma once

#include "graphics/renderers/deferred_renderer.hpp"
#include "imgui_pass.hpp"

namespace violet
{
class deferred_renderer_imgui : public deferred_renderer
{
public:
    void render(render_graph& graph, const render_scene& scene, const render_camera& camera)
        override
    {
        deferred_renderer::render(graph, scene, camera);

        m_imgui_pass.add(
            graph,
            {
                .render_target = get_render_target(),
            });
    }

private:
    imgui_pass m_imgui_pass;
};
} // namespace violet