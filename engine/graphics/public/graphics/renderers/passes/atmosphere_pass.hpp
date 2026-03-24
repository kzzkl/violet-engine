#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class atmosphere_lut_pass
{
public:
    struct parameter
    {
        rdg_texture* sky_view_lut;
        rdg_texture* aerial_perspective_lut;
        rdg_texture* prefilter_map;
        rdg_buffer* irradiance_sh;

        rdg_buffer* vsm_buffer;
        rdg_buffer* vsm_virtual_page_table;
        rdg_texture* vsm_physical_shadow_map;

        bool use_multi_scattering;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void add_sky_view_lut_pass(render_graph& graph, const parameter& parameter);
    void add_aerial_perspective_lut_pass(render_graph& graph, const parameter& parameter);
    void add_environment_map_pass(render_graph& graph, const parameter& parameter);
    void add_irradiance_pass(render_graph& graph, const parameter& parameter);

    rdg_texture* m_environment_map;
};

class atmosphere_pass
{
public:
    struct parameter
    {
        rdg_texture* sky_view_lut;
        rdg_texture* aerial_perspective_lut;
        rdg_texture* render_target;
        rdg_texture* depth_buffer;

        bool clear;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void add_sky_pass(render_graph& graph, const parameter& parameter);
    void add_aerial_perspective_pass(render_graph& graph, const parameter& parameter);
};
} // namespace violet