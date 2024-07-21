#pragma once

#include "graphics/render_interface.hpp"
#include "math/math.hpp"

namespace violet
{
struct rdg_render_pipeline
{
    rhi_shader* vertex_shader;
    rhi_shader* fragment_shader;

    rhi_blend_state blend;
    rhi_depth_stencil_state depth_stencil;
    rhi_rasterizer_state rasterizer;

    rhi_sample_count samples;
    rhi_primitive_topology primitive_topology;
};
} // namespace violet