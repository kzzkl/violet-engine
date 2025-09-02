#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class depth_only_pass
{
public:
    struct parameter
    {
        rdg_buffer* draw_buffer;
        rdg_buffer* draw_count_buffer;
        rdg_buffer* draw_info_buffer;

        rdg_texture* depth_buffer;

        surface_type surface_type;

        bool clear;

        rhi_compare_op depth_compare_op;
        bool stencil_enable;
        rhi_stencil_state stencil_front;
        rhi_stencil_state stencil_back;
        rhi_cull_mode cull_mode;
        rhi_primitive_topology primitive_topology;
    };

    static void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet