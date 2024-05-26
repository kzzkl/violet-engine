#include "components/mesh.hpp"
#include "common/hash.hpp"
#include <cassert>

namespace violet
{
mesh::mesh(render_device* device) : m_geometry(nullptr)
{
    m_parameter = device->create_parameter(engine_parameter_layout::mesh);
}

mesh::~mesh()
{
}

void mesh::set_geometry(geometry* geometry)
{
    m_geometry = geometry;
}

void mesh::add_submesh(
    std::size_t vertex_start,
    std::size_t vertex_count,
    std::size_t index_start,
    std::size_t index_count,
    material* material)
{
    assert(m_geometry != nullptr);

    submesh submesh = {};
    submesh.material = material;
    submesh.vertex_start = vertex_start;
    submesh.vertex_count = vertex_count;
    submesh.index_start = index_start;
    submesh.index_count = index_count;
    submesh.index_buffer = m_geometry->get_index_buffer();

    auto& passes = material->get_layout()->get_passes();
    for (std::size_t i = 0; i < passes.size(); ++i)
    {
        std::hash<std::string> hasher;
        std::size_t hash = 0;
        for (auto& [name, format] : passes[i]->get_input_layout())
            hash = hash_combine(hash, hasher(name));

        auto iter = m_sorted_vertex_buffer.find(hash);
        if (iter == m_sorted_vertex_buffer.end())
        {
            std::vector<rhi_buffer*>& sorted_vertex_buffer = m_sorted_vertex_buffer[hash];
            for (auto& [name, format] : passes[i]->get_input_layout())
                sorted_vertex_buffer.push_back(get_vertex_buffer(name));
            submesh.vertex_buffers.push_back(sorted_vertex_buffer.data());
        }
        else
        {
            submesh.vertex_buffers.push_back(iter->second.data());
        }
    }

    m_submeshes.push_back(submesh);
}

void mesh::set_submesh(
    std::size_t index,
    std::size_t vertex_start,
    std::size_t vertex_count,
    std::size_t index_start,
    std::size_t index_count)
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
    m_parameter->set_uniform(0, &m, sizeof(float4x4), 0);
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