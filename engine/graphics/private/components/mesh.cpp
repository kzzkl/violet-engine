#include "components/mesh.hpp"
#include <cassert>

namespace violet
{
mesh::mesh(renderer* renderer) : m_geometry(nullptr), m_renderer(renderer)
{
    m_parameter = m_renderer->create_parameter(parameter_layout::mesh);
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

    m_submeshes.push_back(submesh);

    auto& passes = material->get_passes();
    for (std::size_t i = 0; i < passes.size(); ++i)
    {
        render_mesh render_mesh = {};
        render_mesh.vertex_start = vertex_start;
        render_mesh.vertex_count = vertex_count;
        render_mesh.index_start = index_start;
        render_mesh.index_count = index_count;
        render_mesh.transform = m_parameter.get();
        render_mesh.material = material->get_parameter(i);
        render_mesh.index_buffer = m_geometry->get_index_buffer();

        for (auto& name : passes[i]->get_vertex_attribute_layout())
            render_mesh.vertex_buffers.push_back(m_geometry->get_vertex_buffer(name));

        m_submeshes.back().render_meshes.push_back(render_mesh);
    }
}

void mesh::set_submesh(
    std::size_t index,
    std::size_t vertex_start,
    std::size_t vertex_count,
    std::size_t index_start,
    std::size_t index_count)
{
    for (render_mesh& mesh : m_submeshes[index].render_meshes)
    {
        mesh.vertex_start = vertex_start;
        mesh.vertex_count = vertex_count;
        mesh.index_start = index_start;
        mesh.index_count = index_count;
    }
}

void mesh::set_model_matrix(const float4x4& m)
{
    m_parameter->set_uniform(0, &m, sizeof(float4x4), 0);
}
} // namespace violet