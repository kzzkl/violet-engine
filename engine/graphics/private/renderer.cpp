#include "graphics/renderer.hpp"
#include "graphics/render_graph/rdg_profiling.hpp"

namespace violet
{
renderer::renderer()
{
    m_profilings.resize(render_device::instance().get_frame_resource_count());
}

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

    if (m_profilings[0] != nullptr)
    {
        auto& device = render_device::instance();
        graph.set_profiling(m_profilings[device.get_frame_resource_index()].get());
    }
}

void renderer::set_profiling(bool enable)
{
    if (enable)
    {
        if (m_profilings[0] != nullptr)
        {
            return;
        }

        for (auto& profiling : m_profilings)
        {
            profiling = std::make_unique<rdg_profiling>();
        }
    }
    else
    {
        for (auto& profiling : m_profilings)
        {
            profiling = nullptr;
        }
    }
}

rdg_profiling* renderer::get_profiling()
{
    if (m_profilings[0] == nullptr)
    {
        return nullptr;
    }

    auto& device = render_device::instance();

    auto* profiling = m_profilings[device.get_frame_resource_index()].get();
    profiling->resolve();

    return profiling;
}
} // namespace violet