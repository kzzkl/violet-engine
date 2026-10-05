#pragma once

#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
struct surface_cache_render_item
{
    std::uint32_t width;
    std::uint32_t height;
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
        std::uint32_t face);
    void copy_surface(render_graph& graph, const surface_cache_render_item& item);

    rdg_texture* m_albedo_buffer{nullptr};
    rdg_texture* m_depth_buffer{nullptr};
};
} // namespace violet