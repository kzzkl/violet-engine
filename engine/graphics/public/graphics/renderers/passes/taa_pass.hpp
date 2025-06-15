#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class taa_pass
{
public:
    struct parameter
    {
        rdg_texture* current_render_target;
        rdg_texture* history_render_target;
        rdg_texture* depth_buffer;
        rdg_texture* motion_vector;
        rdg_texture* resolved_render_target;
    };

    static void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet