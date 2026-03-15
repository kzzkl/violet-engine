#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class prefilter_pass
{
public:
    struct parameter
    {
        rdg_texture* environment_map;
        rdg_texture* prefilter_map;
    };

    void add(render_graph& graph, const parameter& parameter);
};

class irradiance_pass
{
public:
    struct parameter
    {
        rdg_texture* environment_map;
        rdg_buffer* irradiance_sh;
    };

    void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet