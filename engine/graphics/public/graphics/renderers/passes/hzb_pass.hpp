#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class hzb_pass
{
public:
    enum reduction_mode
    {
        REDUCTION_MODE_MIN = 0,
        REDUCTION_MODE_MAX = 1,
    };

    struct parameter
    {
        rdg_texture* depth_buffer;
        rdg_texture* hzb;
        reduction_mode mode = REDUCTION_MODE_MIN;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void add_copy_pass(render_graph& graph, const parameter& parameter);
    void add_reduce_pass(
        render_graph& graph,
        const parameter& parameter,
        std::uint32_t level_offset);
};
} // namespace violet