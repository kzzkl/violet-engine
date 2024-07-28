#pragma once

#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "graphics/render_device.hpp"
#include <vector>

namespace violet
{
struct render_camera
{
    rhi_parameter* parameter;
    std::vector<rhi_texture*> render_targets;

    rhi_viewport viewport;
};

struct render_list
{
    struct mesh
    {
        rhi_parameter* transform;
        geometry* geometry;
    };

    struct draw_item
    {
        std::size_t mesh_index;
        std::size_t parameter_index;

        std::uint32_t vertex_start;
        std::uint32_t index_start;
        std::uint32_t index_count;
    };

    struct batch
    {
        rdg_render_pipeline pipeline;
        std::vector<rhi_parameter*> parameters;

        std::vector<draw_item> items;
    };

    std::vector<mesh> meshes;
    std::vector<batch> batches;

    rhi_parameter* light;
    rhi_parameter* camera;
};

class mesh;
class render_context
{
public:
    render_context();

    render_list get_render_list(const render_camera& camera) const;

private:
    std::vector<mesh*> m_meshes;

    rhi_ptr<rhi_parameter> m_light;

    friend class graphics_module;
};
} // namespace violet