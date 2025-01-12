#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class taa_pass
{
public:
    struct parameter
    {
        rdg_texture* render_target;
        rdg_texture* history_render_target;
        rdg_texture* depth_buffer;
        rdg_texture* motion_vector;

        const render_camera& camera;
    };

    static void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet