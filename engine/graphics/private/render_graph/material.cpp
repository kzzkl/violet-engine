#include "graphics/render_graph/material.hpp"

namespace violet
{
/*material_layout::material_layout()
{
}

void material_layout::add_pipeline(render_pipeline* pipeline)
{
    m_pipelines.push_back(pipeline);
}

material* material_layout::add_material(std::string_view name, renderer* renderer)
{
    m_materials[name.data()] = std::make_unique<material>(this, renderer);
    return m_materials.at(name.data()).get();
}

material* material_layout::get_material(std::string_view name) const
{
    return m_materials.at(name.data()).get();
}*/

material::material()
{
}

material::~material()
{
}
} // namespace violet