#include "graphics/renderers/passes/mesh_pass.hpp"

namespace violet
{
void mesh_pass::add(render_graph& graph, const parameter& parameter)
{
    assert(
        parameter.draw_buffer != nullptr && parameter.draw_count_buffer != nullptr &&
        parameter.draw_info_buffer != nullptr);

    struct pass_data
    {
        rdg_buffer_ref draw_buffer;
        rdg_buffer_ref draw_count_buffer;
        rdg_buffer_srv draw_info_buffer;

        surface_type surface_type;
        material_path material_path;

        rdg_raster_pipeline override_pipeline;

        const render_scene* scene;
    };

    graph.add_pass<pass_data>(
        "Mesh Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            for (const auto& attachment : parameter.render_targets)
            {
                pass.add_render_target(
                    attachment.texture,
                    attachment.load_op,
                    attachment.store_op,
                    0,
                    0,
                    attachment.clear_value);
            }

            if (parameter.depth_buffer.texture != nullptr)
            {
                pass.set_depth_stencil(
                    parameter.depth_buffer.texture,
                    parameter.depth_buffer.load_op,
                    parameter.depth_buffer.store_op,
                    0,
                    0,
                    parameter.depth_buffer.clear_value);
            }

            data.draw_buffer = pass.add_buffer(
                parameter.draw_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
            data.draw_count_buffer = pass.add_buffer(
                parameter.draw_count_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
            data.draw_info_buffer =
                pass.add_buffer_srv(parameter.draw_info_buffer, RHI_PIPELINE_STAGE_VERTEX);

            data.surface_type = parameter.surface_type;
            data.material_path = parameter.material_path;
            data.override_pipeline = parameter.override_pipeline;

            data.scene = &graph.get_scene();
        },
        [](const pass_data& data, rdg_command& command)
        {
            command.set_viewport();
            command.set_scissor();

            auto max_instances = static_cast<std::uint32_t>(
                data.draw_buffer.get_size() / sizeof(shader::draw_command));

            if (data.override_pipeline.vertex_shader == nullptr)
            {
                bool first_batch = true;

                data.scene->each_batch(
                    data.surface_type,
                    data.material_path,
                    [&](render_id id,
                        const rdg_raster_pipeline& pipeline,
                        std::uint32_t instance_offset,
                        std::uint32_t instance_count)
                    {
                        command.set_pipeline(pipeline);

                        if (first_batch)
                        {
                            command.set_constant(
                                mesh_vs::constant_data{
                                    .draw_info_buffer = data.draw_info_buffer.get_bindless(),
                                });
                            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
                            command.set_parameter(1, RDG_PARAMETER_SCENE);
                            command.set_parameter(2, RDG_PARAMETER_CAMERA);
                            command.set_index_buffer();

                            first_batch = false;
                        }

                        command.draw_indexed_indirect(
                            data.draw_buffer.get_rhi(),
                            instance_offset * sizeof(shader::draw_command),
                            data.draw_count_buffer.get_rhi(),
                            id * sizeof(std::uint32_t),
                            std::min(instance_count, max_instances - instance_offset));
                    });
            }
            else
            {
                command.set_pipeline(data.override_pipeline);
                command.set_constant(
                    mesh_vs::constant_data{
                        .draw_info_buffer = data.draw_info_buffer.get_bindless(),
                    });
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);
                command.set_parameter(1, RDG_PARAMETER_SCENE);
                command.set_parameter(2, RDG_PARAMETER_CAMERA);
                command.set_index_buffer();

                data.scene->each_batch(
                    data.surface_type,
                    data.material_path,
                    [&](render_id id,
                        const rdg_raster_pipeline& pipeline,
                        std::uint32_t instance_offset,
                        std::uint32_t instance_count)
                    {
                        command.draw_indexed_indirect(
                            data.draw_buffer.get_rhi(),
                            instance_offset * sizeof(shader::draw_command),
                            data.draw_count_buffer.get_rhi(),
                            id * sizeof(std::uint32_t),
                            std::min(instance_count, max_instances - instance_offset));
                    });
            }
        });
}
} // namespace violet