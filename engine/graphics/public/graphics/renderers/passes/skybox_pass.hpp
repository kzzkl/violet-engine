#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class skybox_pass
{
public:
    struct parameter
    {
        rdg_texture* render_target;
        rdg_texture* depth_buffer;

        bool clear;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void add_sky_view_lut_pass(render_graph& graph, const parameter& parameter);
    void add_aerial_lut_pass(render_graph& graph, const parameter& parameter);
    void add_sky_pass(render_graph& graph, const parameter& parameter);

    rdg_texture* m_sky_view_lut;

    skybox* m_skybox;
};
} // namespace violet