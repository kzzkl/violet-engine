#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class mesh_pass : public rdg_render_pass
{
public:
    struct data : public rdg_data
    {
        render_list render_list;
        rhi_viewport viewport;

        rdg_texture* render_target;
        rdg_texture* depth_buffer;

        bool clear;
    };

public:
    mesh_pass(const data& data);
};
} // namespace violet