#include "ui/rendering/ui_pass.hpp"
#include "ui/rendering/ui_painter.hpp"

namespace violet
{
ui_pass::ui_pass()
{
    rhi_attachment_blend_desc blend = {};
    blend.enable = true;
    blend.src_color_factor = RHI_BLEND_FACTOR_SOURCE_ALPHA;
    blend.dst_color_factor = RHI_BLEND_FACTOR_SOURCE_INV_ALPHA;
    blend.color_op = RHI_BLEND_OP_ADD;
    blend.src_alpha_factor = RHI_BLEND_FACTOR_ONE;
    blend.dst_alpha_factor = RHI_BLEND_FACTOR_ZERO;
    blend.alpha_op = RHI_BLEND_OP_ADD;
    add_color("render target", RHI_TEXTURE_LAYOUT_RENDER_TARGET, blend);

    set_shader("engine/shaders/ui.vert.spv", "engine/shaders/ui.frag.spv");

    set_input_layout({
        {"position", RHI_FORMAT_R32G32_FLOAT  },
        {"color",    RHI_FORMAT_R8G8B8A8_UNORM},
        {"uv",       RHI_FORMAT_R32G32_FLOAT  }
    });

    rhi_depth_stencil_desc depth_stencil_desc = {};
    depth_stencil_desc.depth_enable = false;
    set_depth_stencil(depth_stencil_desc);
    set_cull_mode(RHI_CULL_MODE_NONE);

    set_parameter_layout(
        {ui_painter::get_mvp_parameter_layout(), ui_painter::get_material_parameter_layout()});
}

void ui_pass::execute(rhi_command* command, rdg_context* context)
{
    command->set_render_pipeline(get_pipeline());

    for (ui_painter* painter : m_painters)
    {
        auto vertex_buffers = painter->get_vertex_buffers();
        command->set_vertex_buffers(vertex_buffers.data(), vertex_buffers.size());
        command->set_index_buffer(painter->get_index_buffer());

        command->set_render_parameter(0, painter->get_mvp_parameter());
        painter->each_group(
            [command](ui_draw_group* group)
            {
                if (group->scissor)
                    command->set_scissor(&group->scissor_rect, 1);

                for (auto& mesh : group->meshes)
                {
                    command->set_render_parameter(1, mesh.parameter);
                    command->draw_indexed(mesh.index_start, mesh.index_count, mesh.vertex_start);
                }
            });
    }

    clear_painter();
}
} // namespace violet