#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class depth_only_pass
{
public:
    struct parameter
    {
        rdg_buffer* command_buffer;
        rdg_buffer* count_buffer;

        rdg_texture* depth_buffer;

        material_type material_type;

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