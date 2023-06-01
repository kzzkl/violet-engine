#include "components/mesh.hpp"

namespace violet
{
mesh::mesh()
{
    m_node_parameter = std::make_unique<node_parameter>();
}

void mesh::set_index_buffer(rhi_resource* index_buffer)
{
    m_index_buffer = index_buffer;
}

void mesh::set_vertex_buffer(const std::string& name, rhi_resource* vertex_buffer)
{
    m_vertex_buffers[name] = vertex_buffer;
}

void mesh::remove_vertex_buffer(const std::string& name)
{
    m_vertex_buffers.erase(name);
}

std::size_t mesh::add_submesh(const submesh& submesh)
{
    std::size_t submesh_index = m_submeshes.size();
    m_submeshes.emplace_back(submesh);
    m_materials.resize(submesh_index);

    return submesh_index;
}

void mesh::set_submesh(std::size_t submesh_index, const submesh& submesh)
{
    m_submeshes[submesh_index] = submesh;
}

void mesh::set_material(std::size_t submesh_index, const material& material)
{
    /*
    auto& vertex_layout = material.pipeline->get_vertex_layout();

    m_materials[submesh_index].first = material;
    m_materials[submesh_index].second.clear();
    m_materials[submesh_index].second.reserve(vertex_layout.size());

    for (const std::string& name : vertex_layout)
        m_materials[submesh_index].second.push_back(m_vertex_buffers.at(name));*/
}
} // namespace violet