#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class mesh_pass
{
public:
    struct parameter
    {
        rdg_buffer* command_buffer;
        rdg_buffer* count_buffer;

        std::span<rdg_texture*> render_targets;
        rdg_texture* depth_buffer;

        material_type material_type;

        bool clear;
    };

    static void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet