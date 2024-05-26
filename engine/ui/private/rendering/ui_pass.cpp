#include "ui/rendering/ui_pass.hpp"
#include "ui/rendering/ui_draw_list.hpp"

namespace violet
{
ui_pass::ui_pass()
{
    m_render_target = add_color("render target", RHI_TEXTURE_LAYOUT_RENDER_TARGET);

    set_shader("engine/shaders/ui.vert.spv", "engine/shaders/ui.frag.spv");

    set_input_layout({
        {"position", RHI_FORMAT_R32G32_FLOAT},
        // {"uv",       RHI_FORMAT_R32G32_FLOAT},
        {"color",    RHI_FORMAT_R8G8B8_UNORM}
    });
}

void ui_pass::execute(rhi_command* command, rdg_context* context)
{
    command->set_render_pipeline(get_pipeline());

    for (ui_draw_list* draw_list : m_draw_lists)
    {
        auto vertex_buffers = draw_list->get_vertex_buffers();
        command->set_vertex_buffers(vertex_buffers.data(), vertex_buffers.size());
        command->set_index_buffer(draw_list->get_index_buffer());

        for (auto& mesh : draw_list->get_meshes())
        {
            command->draw_indexed(mesh.index_start, mesh.index_count, mesh.vertex_start);
        }
    }

    clear_draw_list();
}
} // namespace violet