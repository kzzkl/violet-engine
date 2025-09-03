#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class skybox_pass
{
public:
    struct parameter
    {
        rdg_texture* render_target;
        rdg_texture* depth_buffer;

        bool clear;
    };

    void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet