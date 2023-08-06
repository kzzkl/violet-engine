#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
render_graph::render_graph(rhi_context* rhi) : m_rhi(rhi)
{
}

render_resource* render_graph::add_resource(std::string_view name)
{
    return nullptr;
}

render_pass* render_graph::add_render_pass(std::string_view name)
{
    m_render_passes.push_back(std::make_unique<render_pass>(name, m_rhi));
    render_pass* pass = m_render_passes.back().get();

    return pass;
}
} // namespace violet