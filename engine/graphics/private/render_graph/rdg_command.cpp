#include "graphics/render_graph/rdg_command.hpp"
#include "graphics/render_scene.hpp"
#include <algorithm>
#include <cassert>

namespace violet
{
rdg_command::rdg_command(
    rhi_command* command,
    rdg_allocator* allocator,
    const render_scene* scene,
    const render_camera* camera)
    : m_command(command),
      m_allocator(allocator),
      m_scene(scene),
      m_camera(camera)
{
    m_built_in_parameters[RDG_PARAMETER_BINDLESS] =
        render_device::instance().get_bindless_parameter();

    if (scene != nullptr)
    {
        m_built_in_parameters[RDG_PARAMETER_SCENE] = scene->get_scene_parameter();
    }

    if (camera != nullptr)
    {
        m_built_in_parameters[RDG_PARAMETER_CAMERA] = camera->get_camera_parameter();
    }
}

void rdg_command::set_pipeline(const rdg_raster_pipeline& pipeline)
{
    assert(m_render_pass);

    m_command->set_pipeline(render_device::instance().get_pipeline({
        .vertex_shader = pipeline.vertex_shader,
        .fragment_shader = pipeline.fragment_shader,
        .blend = pipeline.blend,
        .depth_stencil = pipeline.depth_stencil,
        .rasterizer = pipeline.rasterizer,
        .samples = pipeline.samples,
        .primitive_topology = pipeline.primitive_topology,
        .render_pass = m_render_pass,
    }));
}

void rdg_command::set_pipeline(const rdg_compute_pipeline& pipeline)
{
    m_command->set_pipeline(render_device::instance().get_pipeline({
        .compute_shader = pipeline.compute_shader,
    }));
}

void rdg_command::set_viewport()
{
    set_viewport(m_camera->get_viewport());
}

void rdg_command::set_scissor()
{
    set_scissor(m_camera->get_scissor_rects());
}

void rdg_command::draw_instances(
    rhi_buffer* command_buffer,
    rhi_buffer* count_buffer,
    material_type type)
{
    assert(m_scene != nullptr && m_camera != nullptr);

    auto& device = render_device::instance();

    for (const auto& batch : m_scene->get_batches())
    {
        if (batch.material_type != type || batch.groups.empty())
        {
            continue;
        }

        set_pipeline(batch.pipeline);

        set_parameter(0, device.get_bindless_parameter());
        set_parameter(1, m_scene->get_scene_parameter());
        set_parameter(2, m_camera->get_camera_parameter());

        for (render_id group_id : batch.groups)
        {
            const auto& group = m_scene->get_group(group_id);

            std::vector<rhi_buffer*> vertex_buffers(group.vertex_buffers.size());
            std::transform(
                group.vertex_buffers.begin(),
                group.vertex_buffers.end(),
                vertex_buffers.begin(),
                [](vertex_buffer* buffer)
                {
                    return buffer->get_rhi();
                });

            set_vertex_buffers(vertex_buffers);
            set_index_buffer(group.index_buffer->get_rhi(), group.index_buffer->get_index_size());

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