#include "components/mesh.hpp"
#include <cassert>

namespace violet
{
mesh::mesh() : m_parameter(nullptr), m_geometry(nullptr)
{
}

mesh::mesh(mesh&& other) noexcept
{
    m_geometry = other.m_geometry;
    m_submeshes = std::move(other.m_submeshes);

    m_parameter = other.m_parameter;
    other.m_parameter = nullptr;
}

mesh::~mesh()
{
    if (m_parameter != nullptr)
        m_geometry->get_context()->get_rhi()->destroy_parameter(m_parameter);
}

void mesh::set_geometry(geometry* geometry)
{
    m_geometry = geometry;

    if (m_parameter == nullptr)
    {
        rhi_renderer* rhi = geometry->get_context()->get_rhi();
        rhi_parameter_layout* layout = geometry->get_context()->get_parameter_layout("violet mesh");
        m_parameter = rhi->create_parameter(layout);
    }
}

void mesh::add_submesh(
    std::size_t vertex_base,
    std::size_t index_start,
    std::size_t index_count,
    material* material)
{
    assert(m_geometry != nullptr);

    submesh submesh = {};
    submesh.vertex_base = vertex_base;
    submesh.index_start = index_start;
    submesh.index_count = index_count;
    submesh.material = material;

    material->each_pipeline(
        [=, this, &submesh](render_pipeline* pipeline, rhi_parameter* parameter)
        {
            render_mesh render_mesh = {};
            render_mesh.vertex_base = vertex_base;
            render_mesh.index_start = index_start;
            render_mesh.index_count = index_count;
            render_mesh.node = m_parameter;
            render_mesh.material = parameter;
            render_mesh.index_buffer = m_geometry->get_index_buffer();

            for (auto [name, format] : pipeline->get_vertex_attributes())
                render_mesh.vertex_buffers.push_back(m_geometry->get_vertex_buffer(name));

            submesh.render_meshes.push_back(render_mesh);
            submesh.render_pipelines.push_back(pipeline);
        });

    m_submeshes.push_back(submesh);
}

void mesh::set_model_matrix(const float4x4& m)
{
    m_parameter->set(0, &m, sizeof(float4x4), 0);
}

mesh& mesh::operator=(mesh&& other) noexcept
{
    m_geometry = other.m_geometry;
    m_submeshes = std::move(other.m_submeshes);

    m_parameter = other.m_parameter;
    other.m_parameter = nullptr;

    return *this;
}
} // namespace violet