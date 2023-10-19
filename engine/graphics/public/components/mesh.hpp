#pragma once

#include "graphics/render_graph/geometry.hpp"
#include "graphics/render_graph/material.hpp"
#include <memory>
#include <vector>

namespace violet
{
class mesh
{
public:
    mesh();
    mesh(const mesh&) = delete;
    mesh(mesh&& other) noexcept;
    ~mesh();

    void set_geometry(geometry* geometry);
    void add_submesh(
        std::size_t vertex_base,
        std::size_t index_start,
        std::size_t index_count,
        material* material);

    void set_model_matrix(const float4x4& m);

    rhi_parameter* get_parameter() const noexcept { return m_parameter; }

    mesh& operator=(const mesh&) = delete;
    mesh& operator=(mesh&& other) noexcept;

    template <typename Functor>
    void each_submesh(Functor functor) const
    {
        for (const submesh& submesh : m_submeshes)
        {
            for (std::size_t i = 0; i < submesh.render_meshes.size(); ++i)
                functor(submesh.render_meshes[i], submesh.render_pipelines[i]);
        }
    }

private:
    struct submesh
    {
        std::size_t vertex_base;
        std::size_t index_start;
        std::size_t index_count;
        material* material;

        std::vector<render_mesh> render_meshes;
        std::vector<render_pipeline*> render_pipelines;
    };

    rhi_parameter* m_parameter;

    geometry* m_geometry;
    std::vector<submesh> m_submeshes;
};
} // namespace violet