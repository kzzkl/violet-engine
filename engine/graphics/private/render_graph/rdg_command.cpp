#include "graphics/render_graph/rdg_command.hpp"
#include <cassert>

namespace violet
{
rdg_command::rdg_command(rhi_command* command, rdg_allocator* allocator)
    : m_command(command),
      m_allocator(allocator)
{
}

void rdg_command::begin_render_pass(rhi_render_pass* render_pass, rhi_framebuffer* framebuffer)
{
    m_command->begin_render_pass(render_pass, framebuffer);
    m_render_pass = render_pass;
    m_subpass_index = 0;
}

void rdg_command::end_render_pass()
{
    m_command->end_render_pass();
    m_render_pass = nullptr;
    m_subpass_index = 0;
}

void rdg_command::next_subpass()
{
    m_command->next_subpass();
    ++m_subpass_index;
}

void rdg_command::set_pipeline(const rdg_render_pipeline& pipeline)
{
    assert(m_render_pass);
    m_command->set_pipeline(m_allocator->get_pipeline(pipeline, m_render_pass, m_subpass_index));
}

void rdg_command::draw_render_list(const render_list& render_list)
{
    for (auto& batch : render_list.batches)
    {
        geometry* geometry = nullptr;

        set_pipeline(batch.pipeline);
        set_parameter(2, render_list.camera);
        set_parameter(3, render_list.light);

        for (auto& item : batch.items)
        {
            set_parameter(0, render_list.meshes[item.mesh_index].transform);
            set_parameter(1, batch.parameters[item.parameter_index]);

            if (geometry != render_list.meshes[item.mesh_index].geometry)
            {
                geometry = render_list.meshes[item.mesh_index].geometry;

                std::vector<rhi_buffer*> vertex_buffers;
                for (std::size_t i = 0; i < batch.pipeline.input.vertex_attribute_count; ++i)
                {
                    vertex_buffers.push_back(geometry->get_vertex_buffer(
                        batch.pipeline.input.vertex_attributes[i].name));
                }

                set_vertex_buffers(vertex_buffers);
                set_index_buffer(geometry->get_index_buffer());
            }

            draw_indexed(item.index_start, item.index_count, item.vertex_start);
        }
    }
}
} // namespace violet