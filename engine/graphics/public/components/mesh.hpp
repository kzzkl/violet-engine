#pragma once

#include "graphics/geometry.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "math/math.hpp"
#include <memory>
#include <vector>

namespace violet
{
class material;
class mesh
{
public:
    struct submesh
    {
        material* material;
        std::vector<render_mesh> render_meshes;
    };

public:
    mesh(renderer* renderer);
    mesh(const mesh&) = delete;
    mesh(mesh&& other) = default;
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

    const std::vector<submesh>& get_submeshes() const noexcept { return m_submeshes; }

    void set_model_matrix(const float4x4& m);

    mesh& operator=(const mesh&) = delete;
    mesh& operator=(mesh&& other) = default;

private:
    rhi_ptr<rhi_parameter> m_parameter;

    geometry* m_geometry;
    std::vector<submesh> m_submeshes;

    renderer* m_renderer;
};
} // namespace violet