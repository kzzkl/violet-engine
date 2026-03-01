#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class taa_pass
{
public:
    struct parameter
    {
        rdg_texture* current_render_target;
        rdg_texture* history_render_target;
        rdg_texture* depth_buffer;
        rdg_texture* motion_vector;
        bool history_valid;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void resolve(render_graph& graph, const parameter& parameter);
};
} // namespace violet