#pragma once

#include "graphics/render_interface.hpp"

namespace violet
{
struct rdg_raster_pipeline
{
    rhi_shader* vertex_shader;
    rhi_shader* geometry_shader;
    rhi_shader* fragment_shader;

    rhi_rasterizer_state* rasterizer_state;
    rhi_depth_stencil_state* depth_stencil_state;
    rhi_blend_state* blend_state;

    rhi_primitive_topology primitive_topology;
    rhi_sample_count samples;

    bool operator==(const rdg_raster_pipeline& other) const noexcept
    {
        return vertex_shader == other.vertex_shader && geometry_shader == other.geometry_shader &&
               fragment_shader == other.fragment_shader &&
               rasterizer_state == other.rasterizer_state &&
               depth_stencil_state == other.depth_stencil_state &&
               blend_state == other.blend_state && primitive_topology == other.primitive_topology &&
               samples == other.samples;
    }

    rhi_raster_pipeline_desc get_desc(rhi_render_pass* render_pass) const noexcept
    {
        rhi_raster_pipeline_desc desc = {};
        desc.vertex_shader = vertex_shader;
        desc.geometry_shader = geometry_shader;
        desc.fragment_shader = fragment_shader;
        desc.rasterizer_state = rasterizer_state;
        desc.depth_stencil_state = depth_stencil_state;
        desc.blend_state = blend_state;
        desc.primitive_topology = primitive_topology;
        desc.samples = samples;
        desc.render_pass = render_pass;
        return desc;
    }
};

struct rdg_compute_pipeline
{
    rhi_shader* compute_shader;

    bool operator==(const rdg_compute_pipeline& other) const noexcept
    {
        return compute_shader == other.compute_shader;
    }

    rhi_compute_pipeline_desc get_desc() const noexcept
    {
        return {
            .compute_shader = compute_shader,
        };
    }
};
} // namespace violet