#include "graphics/renderer.hpp"

namespace violet
{
void renderer::render(render_graph& graph)
{
    if (m_feature_dirty)
    {
        m_feature_dirty = false;

        m_enabled_features.clear();

        for (const auto& feature : m_features)
        {
            if (feature->is_enable())
            {
                m_enabled_features.push_back(feature.get());
            }
        }
    }

    rhi_texture_extent extent = graph.get_camera().get_render_target()->get_extent();
    for (const auto& feature : m_enabled_features)
    {
        feature->update(extent.width, extent.height);
    }

    on_render(graph);
}
} // namespace violet