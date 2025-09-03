#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class gtao_pass
{
public:
    struct parameter
    {
        std::uint32_t slice_count;
        std::uint32_t step_count;
        float radius;
        float falloff;

        rdg_texture* hzb;
        rdg_texture* depth_buffer;
        rdg_texture* normal_buffer;
        rdg_texture* ao_buffer;
    };

    void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet