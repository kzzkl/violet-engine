#pragma once

#include "graphics/render_graph/render_graph.hpp"
#include "graphics/render_scene.hpp"

namespace violet
{
class skybox_pass
{
public:
    struct parameter
    {
        const render_scene& scene;
        const render_camera& camera;

        rdg_texture* render_target;
        rdg_texture* depth_buffer;

        bool clear;
    };

    static void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet