#include "graphics/graphics_context.hpp"
#include <cassert>

namespace violet
{
graphics_context::graphics_context(rhi_renderer* rhi) : m_rhi(rhi)
{
    add_parameter_layout(
        "violet mesh",
        {
            {RHI_PARAMETER_TYPE_UNIFORM_BUFFER,
             sizeof(float4x4),
             RHI_PARAMETER_FLAG_VERTEX | RHI_PARAMETER_FLAG_FRAGMENT}
    });
    add_parameter_layout(
        "violet camera",
        {
            {RHI_PARAMETER_TYPE_UNIFORM_BUFFER,
             sizeof(float4x4) * 3,
             RHI_PARAMETER_FLAG_VERTEX | RHI_PARAMETER_FLAG_FRAGMENT}
    });
}

graphics_context::~graphics_context()
{
    for (auto& [name, layout] : m_parameter_layouts)
        m_rhi->destroy_parameter_layout(layout);
}

rhi_parameter_layout* graphics_context::add_parameter_layout(
    std::string_view name,
    const std::vector<rhi_parameter_layout_pair>& layout)
{
    assert(m_parameter_layouts.find(name.data()) == m_parameter_layouts.end());

    rhi_parameter_layout_desc desc = {};
    for (std::size_t i = 0; i < layout.size(); ++i)
        desc.parameters[i] = layout[i];
    desc.parameter_count = layout.size();

    rhi_parameter_layout* interface = m_rhi->create_parameter_layout(desc);
    m_parameter_layouts[name.data()] = interface;
    return interface;
}

rhi_parameter_layout* graphics_context::get_parameter_layout(std::string_view name) const
{
    return m_parameter_layouts.at(name.data());
}
} // namespace violet