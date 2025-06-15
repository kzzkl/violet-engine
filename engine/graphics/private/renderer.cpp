#include "graphics/renderer.hpp"

namespace violet
{
void renderer::render(render_graph& graph)
{
    rhi_texture_extent extent = graph.get_camera().get_render_target()->get_extent();
    for (const auto& feature : m_features)
    {
        if (feature->is_enable())
        {
            feature->update(extent.width, extent.height);
        }
    }

    on_render(graph);
}
} // namespace violet