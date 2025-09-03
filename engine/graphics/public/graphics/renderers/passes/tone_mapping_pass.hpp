#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class tone_mapping_pass
{
public:
    struct parameter
    {
        rdg_texture* hdr_texture;
        rdg_texture* ldr_texture;
    };

    void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet