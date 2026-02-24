#include "graphics/renderer.hpp"
#include "graphics/render_graph/rdg_profiling.hpp"

namespace violet
{
renderer::renderer() = default;

renderer::~renderer() {}

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

    if (m_profiling != nullptr)
    {
        graph.set_profiling(m_profiling.get());
    }
}

void renderer::set_profiling(bool enable)
{
    if (enable)
    {
        if (m_profiling != nullptr)
        {
            return;
        }

        m_profiling = std::make_unique<rdg_profiling>();
    }
    else
    {
        m_profiling = nullptr;
    }
}

rdg_profiling* renderer::get_profiling()
{
    if (m_profiling == nullptr)
    {
        return nullptr;
    }

    m_profiling->resolve();

    return m_profiling.get();
}
} // namespace violet