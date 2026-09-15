#pragma once

#include "graphics/render_graph/render_graph.hpp"
#include "math/box.hpp"

namespace violet
{
class sdf_debug_pass
{
public:
    struct parameter
    {
        box3f bounding_box;
        mat4f matrix_m;

        rhi_texture* sdf;
        rdg_texture* render_target;
        rdg_texture* depth_buffer;

        bool render_bounds;
        bool render_bricks;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void add_sdf_pass(render_graph& graph, const parameter& parameter);
    void add_bounds_pass(render_graph& graph, const parameter& parameter);
};
} // namespace violet