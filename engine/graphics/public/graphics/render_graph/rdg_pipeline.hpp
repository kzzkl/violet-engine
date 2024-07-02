#pragma once

#include "graphics/render_interface.hpp"

namespace violet
{
struct rdg_render_pipeline
{
    rhi_shader* vertex_shader;
    rhi_shader* fragment_shader;

    rhi_input_state input;
    rhi_blend_state blend;
    rhi_depth_stencil_state depth_stencil;
    rhi_rasterizer_state rasterizer;

    rhi_sample_count samples;
    rhi_primitive_topology primitive_topology;

    rhi_parameter_desc parameters[rhi_constants::MAX_PARAMETER_COUNT];
    std::size_t parameter_count;
};
} // namespace violet