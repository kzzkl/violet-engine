#include "graphics/render_graph/rdg_command.hpp"
#include <algorithm>
#include <cassert>
#include <span>

namespace violet
{
rdg_command::rdg_command(rhi_command* command, rdg_allocator* allocator)
    : m_command(command),
      m_allocator(allocator)
{
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
        set_parameter(0, render_list.camera);
        set_parameter(1, render_list.light);

        for (auto& item : batch.items)
        {
            set_parameter(2, render_list.meshes[item.mesh_index].transform);

            if (batch.parameters[item.parameter_index] != nullptr)
                set_parameter(3, batch.parameters[item.parameter_index]);

            if (geometry != render_list.meshes[item.mesh_index].geometry)
            {
                geometry = render_list.meshes[item.mesh_index].geometry;

                auto& attributes =
                    render_device::instance().get_vertex_attributes(batch.pipeline.vertex_shader);
                std::vector<rhi_buffer*> vertex_buffers(attributes.size());
                for (std::size_t i = 0; i < attributes.size(); ++i)
                    vertex_buffers[i] = geometry->get_vertex_buffer(attributes[i]);

                set_vertex_buffers(vertex_buffers);
                set_index_buffer(geometry->get_index_buffer());
            }

            draw_indexed(item.index_start, item.index_count, item.vertex_start);
        }
    }
}
} // namespace violet