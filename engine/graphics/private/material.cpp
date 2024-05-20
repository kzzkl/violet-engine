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
        auto& parameter_layout = pass->get_parameter_layout();
        std::size_t material_parameter_index = pass->get_material_parameter_index();
        if (material_parameter_index != -1)
            m_parameters.push_back(
                device->create_parameter(parameter_layout[material_parameter_index]));
        else
            m_parameters.push_back(nullptr);
    }
}
} // namespace violet