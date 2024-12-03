#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class mesh_pass : public rdg_render_pass
{
public:
    struct parameter
    {
        const render_scene& scene;
        const render_camera& camera;

        rdg_buffer* command_buffer;
        rdg_buffer* count_buffer;

        rdg_texture* gbuffer_albedo;
        rdg_texture* depth_buffer;

        bool clear;
    };

    static void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet