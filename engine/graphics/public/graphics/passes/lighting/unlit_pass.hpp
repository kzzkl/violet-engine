#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class unlit_pass
{
public:
    struct parameter
    {
        rdg_texture* gbuffer_albedo;

        rdg_texture* depth_buffer;
        rdg_texture* render_target;

        bool clear;
    };

    static void add(render_graph& graph, const render_context& context, const parameter& parameter);
};
} // namespace violet