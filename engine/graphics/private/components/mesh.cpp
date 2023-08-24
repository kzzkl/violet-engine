#include "components/mesh.hpp"

namespace violet
{
mesh::mesh()
{
}

void mesh::set_geometry(geometry* geometry)
{
    m_geometry = geometry;
}

void mesh::set_submesh(std::size_t index, const submesh& submesh)
{
    m_submeshes[index] = submesh;
}

void mesh::set_submesh_count(std::size_t count)
{
    m_submeshes.resize(count);
}

void mesh::set_material(std::size_t submesh_index, material* material)
{
    /*auto& pipelines = material->get_pipelines();

    for (auto [pipeline, parameter] : pipelines)
    {
        auto& vertex_layout = pipeline->get_vertex_layout();
    }*/

    /*
    auto& vertex_layout = material.pipeline->get_vertex_layout();

    m_materials[submesh_index].first = material;
    m_materials[submesh_index].second.clear();
    m_materials[submesh_index].second.reserve(vertex_layout.size());

    for (const std::string& name : vertex_layout)
        m_materials[submesh_index].second.push_back(m_vertex_buffers.at(name));*/
}

material* mesh::get_material(std::size_t index)
{
    return m_materials[index].material;
}

void mesh::set_material_count(std::size_t count)
{
    m_materials.resize(count);
}

std::size_t mesh::get_material_count() const noexcept
{
    return m_materials.size();
}
} // namespace violet