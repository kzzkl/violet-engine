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

void rdg_command::set_pipeline(const rdg_compute_pipeline& pipeline)
{
    m_command->set_pipeline(m_allocator->get_pipeline(pipeline));
}

void rdg_command::draw_instances(
    const render_scene& scene,
    const render_camera& camera,
    rhi_buffer* command_buffer,
    rhi_buffer* count_buffer,
    material_type type)
{
    auto& device = render_device::instance();

    for (auto& batch : scene.get_batches())
    {
        if (batch.material_type != type || batch.groups.empty())
        {
            continue;
        }

        set_pipeline(batch.pipeline);

        set_parameter(0, scene.get_global_parameter());
        set_parameter(1, scene.get_scene_parameter());
        set_parameter(2, camera.camera_parameter);

        for (render_id group_id : batch.groups)
        {
            auto& group = scene.get_group(group_id);

            set_vertex_buffers(group.vertex_buffers);
            set_index_buffer(group.index_buffer);

            m_command->draw_indexed_indirect(
                command_buffer,
                group.instance_offset * sizeof(shader::draw_command),
                count_buffer,
                group.id * sizeof(std::uint32_t),
                group.instance_count);
        }
    }
}
} // namespace violet