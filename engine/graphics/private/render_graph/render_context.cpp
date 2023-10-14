#include "graphics/render_graph/render_context.hpp"
#include <cassert>

namespace violet
{
render_context::render_context(rhi_renderer* rhi) : m_rhi(rhi)
{
}

render_context::~render_context()
{
    for (auto& [name, layout] : m_parameter_layouts)
        m_rhi->destroy_parameter_layout(layout);
}

rhi_parameter_layout* render_context::add_parameter_layout(
    std::string_view name,
    const std::vector<std::pair<rhi_parameter_type, std::size_t>>& layout)
{
    assert(get_parameter_layout(name) == nullptr);

    rhi_parameter_layout_desc desc = {};
    for (std::size_t i = 0; i < layout.size(); ++i)
    {
        desc.parameters[i].type = layout[i].first;
        desc.parameters[i].size = layout[i].second;
    }
    desc.parameter_count = layout.size();

    rhi_parameter_layout* interface = m_rhi->create_parameter_layout(desc);
    m_parameter_layouts[name.data()] = interface;
    return interface;
}

rhi_parameter_layout* render_context::get_parameter_layout(std::string_view name) const
{
    auto iter = m_parameter_layouts.find(name.data());
    if (iter == m_parameter_layouts.end())
        return nullptr;
    else
        return iter->second;
}
} // namespace violet