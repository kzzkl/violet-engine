#pragma once

#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "math/math.hpp"
#include <map>
#include <memory>
#include <vector>

namespace violet
{
class mesh
{
public:
    struct submesh
    {
        material* material;
        std::uint32_t vertex_start;
        std::uint32_t vertex_count;
        std::uint32_t index_start;
        std::uint32_t index_count;
    };

public:
    mesh();
    mesh(const mesh&) = delete;
    mesh(mesh&&) = default;
    ~mesh();

    void set_geometry(geometry* geometry);
    geometry* get_geometry() const noexcept { return m_geometry; }

    void add_submesh(
        std::uint32_t vertex_start,
        std::uint32_t vertex_count,
        std::uint32_t index_start,
        std::uint32_t index_count,
        material* material);

    void set_submesh(
        std::size_t index,
        std::uint32_t vertex_start,
        std::uint32_t vertex_count,
        std::uint32_t index_start,
        std::uint32_t index_count);

    const std::vector<submesh>& get_submeshes() const noexcept { return m_submeshes; }

    void set_skinned_vertex_buffer(std::string_view name, rhi_buffer* vertex_buffer);

    void set_model_matrix(const float4x4& m);

    rhi_parameter* get_mesh_parameter() const noexcept { return m_parameter.get(); }

    mesh& operator=(const mesh&) = delete;
    mesh& operator=(mesh&&) = default;

private:
    rhi_buffer* get_vertex_buffer(std::string_view name);

    rhi_ptr<rhi_parameter> m_parameter;

    geometry* m_geometry;
    std::vector<submesh> m_submeshes;

    std::map<std::string, rhi_buffer*> m_skinned_vertex_buffer;
    std::map<std::size_t, std::vector<rhi_buffer*>> m_sorted_vertex_buffer;
};
} // namespace violet