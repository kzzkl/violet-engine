#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class motion_vector_pass
{
public:
    struct parameter
    {
        rdg_texture* depth_buffer;
        rdg_texture* motion_vector;
    };

    static void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet