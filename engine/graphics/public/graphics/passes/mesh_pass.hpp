#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class mesh_pass
{
public:
    static void render(
        render_graph& graph,
        const render_context& context,
        const render_camera& camera,
        rdg_texture* render_target,
        rdg_texture* depth_buffer);
};
} // namespace violet