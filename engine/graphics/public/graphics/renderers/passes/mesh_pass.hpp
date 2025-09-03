#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class mesh_pass
{
public:
    struct parameter
    {
        rdg_buffer* draw_buffer;
        rdg_buffer* draw_count_buffer;
        rdg_buffer* draw_info_buffer;

        std::span<rdg_texture*> render_targets;
        rdg_texture* depth_buffer;

        surface_type surface_type;
        material_path material_path;

        bool clear;
        std::span<rhi_clear_value> render_target_clear_values;
        rhi_clear_value depth_buffer_clear_value;

        rdg_raster_pipeline override_pipeline;
    };

    void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet