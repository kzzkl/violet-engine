#include "components/mesh.hpp"
#include "common/hash.hpp"
#include <cassert>

namespace violet
{
mesh::mesh() : m_geometry(nullptr)
{
    m_parameter = render_device::instance().create_parameter(shader::mesh);
}

mesh::~mesh()
{
}

void mesh::set_geometry(geometry* geometry)
{
    m_geometry = geometry;
}

void mesh::add_submesh(
    std::uint32_t vertex_start,
    std::uint32_t vertex_count,
    std::uint32_t index_start,
    std::uint32_t index_count,
    material* material)
{
    submesh submesh = {};
    submesh.material = material;
    submesh.vertex_start = vertex_start;
    submesh.vertex_count = vertex_count;
    submesh.index_start = index_start;
    submesh.index_count = index_count;

    m_submeshes.push_back(submesh);
}

void mesh::set_submesh(
    std::size_t index,
    std::uint32_t vertex_start,
    std::uint32_t vertex_count,
    std::uint32_t index_start,
    std::uint32_t index_count)
{
    m_submeshes[index].vertex_start = vertex_start;
    m_submeshes[index].vertex_count = vertex_count;
    m_submeshes[index].index_start = index_start;
    m_submeshes[index].index_count = index_count;
}

void mesh::set_skinned_vertex_buffer(std::string_view name, rhi_buffer* vertex_buffer)
{
    rhi_buffer* original_vertex_buffer = get_vertex_buffer(name);
    if (original_vertex_buffer == nullptr)
    {
        m_skinned_vertex_buffer[name.data()] = vertex_buffer;
        return;
    }

    for (auto& [hash, buffers] : m_sorted_vertex_buffer)
    {
        for (auto& buffer : buffers)
        {
            if (buffer == original_vertex_buffer)
                buffer = vertex_buffer;
        }
    }
}

void mesh::set_model_matrix(const float4x4& m)
{
    m_parameter->set_uniform(0, &m, sizeof(float4x4), offsetof(shader::mesh_data, model));
    m_parameter->set_uniform(0, &m, sizeof(float4x4), offsetof(shader::mesh_data, normal));
}

rhi_buffer* mesh::get_vertex_buffer(std::string_view name)
{
    auto iter = m_skinned_vertex_buffer.find(name.data());
    if (iter != m_skinned_vertex_buffer.end())
        return iter->second;
    else if (m_geometry != nullptr)
        return m_geometry->get_vertex_buffer(name);
    else
        return nullptr;
}
} // namespace violet