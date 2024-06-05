#include "graphics/material.hpp"

namespace violet
{
material_layout::material_layout(render_graph* render_graph, const std::vector<rdg_pass*>& passes)
    : m_render_graph(render_graph),
      m_passes(passes)
{
}

material::material(render_device* device, material_layout* layout) : m_layout(layout)
{
    for (rdg_pass* pass : layout->get_passes())
    {
        auto parameter_layout = pass->get_parameter_layout(RDG_PASS_PARAMETER_FLAG_MATERIAL);
        if (!parameter_layout.empty())
            m_parameters.push_back(device->create_parameter(parameter_layout[0]));
        else
            m_parameters.push_back(nullptr);
    }
}
} // namespace violet