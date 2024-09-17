#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class skybox_pass : public rdg_render_pass
{
public:
    struct data : public rdg_data
    {
        rhi_parameter* camera;
        rhi_viewport viewport;

        rdg_texture* render_target;
        rdg_texture* depth_buffer;

        bool clear;
    };

public:
    skybox_pass(const data& data);
};
} // namespace violet