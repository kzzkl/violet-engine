#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class unlit_pass
{
public:
    struct parameter
    {
        const render_scene& scene;

        rdg_texture* gbuffer_albedo;
        rdg_texture* gbuffer_depth;

        rdg_texture* render_target;

        bool clear;
    };

    static void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet