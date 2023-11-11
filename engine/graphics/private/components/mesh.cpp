#include "components/mesh.hpp"
#include <cassert>

namespace violet
{
mesh::mesh(rhi_renderer* rhi, rhi_parameter_layout* mesh_parameter_layout)
    : m_geometry(nullptr),
      m_rhi(rhi)
{
    m_parameter = m_rhi->create_parameter(mesh_parameter_layout);
}

mesh::mesh(mesh&& other) noexcept
{
    *this = std::move(other);
}

mesh::~mesh()
{
    if (m_parameter)
        m_rhi->destroy_parameter(m_parameter);
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
    material->each_pipeline(
        [=, this](render_pipeline* pipeline, rhi_parameter* parameter)
        {
            render_mesh render_mesh = {};
            render_mesh.vertex_start = vertex_start;
            render_mesh.vertex_count = vertex_count;
            render_mesh.index_start = index_start;
            render_mesh.index_count = index_count;
            render_mesh.transform = m_parameter;
            render_mesh.material = parameter;
            render_mesh.index_buffer = m_geometry->get_index_buffer();

            for (auto [name, format] : pipeline->get_vertex_attributes())
                render_mesh.vertex_buffers.push_back(m_geometry->get_vertex_buffer(name));

            m_submeshes.back().render_meshes.push_back(render_mesh);
            m_submeshes.back().render_pipelines.push_back(pipeline);
        });
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

mesh& mesh::operator=(mesh&& other) noexcept
{
    m_parameter = other.m_parameter;
    m_geometry = other.m_geometry;
    m_submeshes = std::move(other.m_submeshes);
    m_rhi = other.m_rhi;

    other.m_parameter = nullptr;
    other.m_rhi = nullptr;

    return *this;
}
} // namespace violet