#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class hzb_pass
{
public:
    struct parameter
    {
        rdg_texture* depth_buffer;
        rdg_texture* hzb;
    };

    static void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet