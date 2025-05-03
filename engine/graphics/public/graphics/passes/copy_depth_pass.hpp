#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class copy_depth_pass
{
public:
    struct parameter
    {
        rdg_texture* src;
        rdg_texture* dst;
    };

    static void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet