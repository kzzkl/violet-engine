#pragma once

#include "graphics/geometry.hpp"
#include "graphics/render_graph/material.hpp"
#include <memory>
#include <vector>

namespace violet
{
class mesh
{
public:
    mesh(rhi_renderer* rhi, rhi_parameter_layout* mesh_parameter_layout);
    mesh(const mesh&) = delete;
    mesh(mesh&& other) noexcept;
    ~mesh();

    void set_geometry(geometry* geometry);
    geometry* get_geometry() const noexcept { return m_geometry; }

    void add_submesh(
        std::size_t vertex_start,
        std::size_t vertex_count,
        std::size_t index_start,
        std::size_t index_count,
        material* material);

    void set_submesh(
        std::size_t index,
        std::size_t vertex_start,
        std::size_t vertex_count,
        std::size_t index_start,
        std::size_t index_count);

    void set_model_matrix(const float4x4& m);

    rhi_parameter* get_parameter() const noexcept { return m_parameter; }

    template <typename Functor>
    void each_submesh(Functor functor) const
    {
        for (const submesh& submesh : m_submeshes)
        {
            for (std::size_t i = 0; i < submesh.render_meshes.size(); ++i)
                functor(submesh.render_meshes[i], submesh.render_pipelines[i]);
        }
    }

    mesh& operator=(const mesh&) = delete;
    mesh& operator=(mesh&& other) noexcept;

private:
    struct submesh
    {
        material* material;
        std::vector<render_mesh> render_meshes;
        std::vector<render_pipeline*> render_pipelines;
    };

    rhi_parameter* m_parameter;

    geometry* m_geometry;
    std::vector<submesh> m_submeshes;

    rhi_renderer* m_rhi;
};
} // namespace violet