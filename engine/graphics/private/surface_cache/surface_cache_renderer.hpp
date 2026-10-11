#pragma once

#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "surface_cache/surface_cache_common.hpp"

namespace violet
{
struct surface_cache_render_item
{
    struct face
    {
        std::uint32_t width;
        std::uint32_t height;
        std::vector<vec2u> page_src_coords;
        std::vector<vec2u> page_dst_coords;
        std::vector<vec2u> page_extents;
    };

    std::array<face, SURFACE_CACHE_FACE_COUNT> faces;

    geometry* geometry;
    std::vector<std::pair<std::uint32_t, material*>> materials;
};

class surface_cache_renderer
{
public:
    void render(
        render_graph& graph,
        std::span<surface_cache_render_item> items,
        rhi_texture* albedo_buffer,
        rhi_texture* depth_buffer);

private:
    void render_surface(
        render_graph& graph,
        geometry* geometry,
        material* material,
        std::uint32_t submesh_index,
        std::uint32_t face,
        std::uint32_t width,
        std::uint32_t height,
        bool clear);
    void copy_surface(render_graph& graph, const surface_cache_render_item::face& face);

    rdg_texture* m_albedo_temp{nullptr};
    rdg_texture* m_depth_temp{nullptr};

    rdg_texture* m_albedo_buffer{nullptr};
    rdg_texture* m_depth_buffer{nullptr};
};
} // namespace violet