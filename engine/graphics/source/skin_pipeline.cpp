#include "skin_pipeline.hpp"

namespace ash::graphics
{
void skin_pipeline::add(const skinned_mesh& skinned_mesh)
{
    skin_unit unit = {};
    unit.input_buffers = skinned_mesh.vertex_buffers;
    for (auto& skinned : skinned_mesh.skinned_vertex_buffers)
        unit.output_buffers.push_back(skinned.get());
    unit.parameter = skinned_mesh.parameter.get();
    unit.vertex_count = skinned_mesh.vertex_count;
    m_units.push_back(unit);
}
} // namespace ash::graphics