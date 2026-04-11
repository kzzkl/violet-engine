#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class dithering_pass
{
public:
    struct parameter
    {
        rdg_texture* render_target;
        std::uint32_t frame;
    };

    void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet