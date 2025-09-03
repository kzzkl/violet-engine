#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class blit_pass
{
public:
    struct parameter
    {
        rdg_texture* src;
        rhi_texture_region src_region;
        rdg_texture* dst;
        rhi_texture_region dst_region;
        rhi_filter filter;
    };

    void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet