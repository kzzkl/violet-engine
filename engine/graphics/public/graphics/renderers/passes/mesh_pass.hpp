#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class mesh_pass
{
public:
    struct attachment
    {
        rdg_texture* texture{nullptr};
        rhi_attachment_store_op store_op{RHI_ATTACHMENT_STORE_OP_STORE};
        rhi_attachment_load_op load_op{RHI_ATTACHMENT_LOAD_OP_LOAD};
        rhi_clear_value clear_value{};
    };

    struct parameter
    {
        rdg_buffer* draw_buffer;
        rdg_buffer* draw_count_buffer;
        rdg_buffer* draw_info_buffer;

        std::span<attachment> render_targets;
        attachment depth_buffer;

        surface_type surface_type;
        material_path material_path;

        rdg_raster_pipeline override_pipeline;
    };

    void add(render_graph& graph, const parameter& parameter);
};
} // namespace violet