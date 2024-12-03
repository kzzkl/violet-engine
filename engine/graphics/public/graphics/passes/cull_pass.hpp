#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class cull_pass
{
public:
    struct parameter
    {
        const render_scene& scene;
        const render_camera& camera;

        rdg_buffer* command_buffer;
        rdg_buffer* count_buffer;

        bool frustum_culling;
        bool occlusion_culling;
    };

    static void add(render_graph& graph, const parameter& parameter);

private:
    static void add_reset_pass(render_graph& graph, const parameter& parameter);
    static void add_cull_pass(
        render_graph& graph,
        const parameter& parameter,
        rdg_buffer* cull_result);
    static void add_fill_pass(
        render_graph& graph,
        const parameter& parameter,
        rdg_buffer* cull_result);
};
} // namespace violet