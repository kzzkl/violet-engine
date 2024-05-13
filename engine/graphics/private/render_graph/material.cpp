#include "graphics/render_graph/material.hpp"

namespace violet
{
material_layout::material_layout(const std::vector<mesh_pass*>& passes) : m_passes(passes)
{
}

material::material(renderer* renderer, material_layout* layout) : m_layout(layout)
{
    for (mesh_pass* pass : layout->get_passes())
        m_parameters.push_back(renderer->create_parameter(pass->get_material_parameter_desc()));
}
} // namespace violet