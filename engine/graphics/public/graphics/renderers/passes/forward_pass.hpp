#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class forward_pass
{
public:
    struct parameter
    {
        rdg_buffer* draw_buffer;
        rdg_buffer* draw_count_buffer;
        rdg_buffer* draw_info_buffer;

        rdg_texture* render_target;
        rdg_texture* depth_buffer;

        bool main_pass;
    };

    void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet