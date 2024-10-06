#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class mesh_pass : public rdg_render_pass
{
public:
    struct parameter
    {
        render_list render_list;
        rhi_viewport viewport;

        rdg_texture* render_target;
        rdg_texture* depth_buffer;

        bool clear;
    };

public:
    static void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet