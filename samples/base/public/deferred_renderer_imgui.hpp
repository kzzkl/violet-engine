#pragma once

#include "graphics/renderers/deferred_renderer.hpp"
#include "imgui_pass.hpp"

namespace violet
{
class deferred_renderer_imgui : public deferred_renderer
{
protected:
    void on_render(render_graph& graph) override
    {
        deferred_renderer::on_render(graph);

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