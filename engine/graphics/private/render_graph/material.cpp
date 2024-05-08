#include "graphics/render_graph/material.hpp"

namespace violet
{
material_layout::material_layout(const std::vector<mesh_pass*>& passes) : m_passes(passes)
{
}

material::material(material_layout* layout) : m_layout(layout)
{
}
} // namespace violet