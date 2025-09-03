#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class bounds_projection_pass
{
public:
    struct parameter
    {
        rdg_texture* render_target;
    };

    void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet