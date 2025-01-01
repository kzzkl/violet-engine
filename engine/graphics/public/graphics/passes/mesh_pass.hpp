#pragma once

#include "graphics/render_graph/render_graph.hpp"
#include "graphics/render_scene.hpp"

namespace violet
{
class mesh_pass
{
public:
    struct parameter
    {
        const render_scene& scene;
        const render_camera& camera;

        rdg_buffer* command_buffer;
        rdg_buffer* count_buffer;

        rdg_texture* gbuffer_albedo;
        rdg_texture* gbuffer_material;
        rdg_texture* gbuffer_normal;
        rdg_texture* gbuffer_emissive;
        rdg_texture* depth_buffer;

        bool clear;
    };

    static void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet