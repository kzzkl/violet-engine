#include "graphics/passes/cull_pass.hpp"

namespace violet
{
struct cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/source/cull.hlsl";

    static constexpr parameter parameter = {
        {RHI_PARAMETER_STORAGE, RHI_SHADER_STAGE_COMPUTE, 1}, // Cull Result
    };

    static constexpr parameter_layout parameters = {
        {0, parameter},
        {1, scene},
        {2, camera},
    };
};

struct draw_command_filler_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/source/draw_command_filler.hlsl";

    static constexpr parameter parameter = {
        {RHI_PARAMETER_STORAGE, RHI_SHADER_STAGE_COMPUTE, 1},       // Group Buffer
        {RHI_PARAMETER_STORAGE, RHI_SHADER_STAGE_COMPUTE, 1},       // Cull Result
        {RHI_PARAMETER_STORAGE, RHI_SHADER_STAGE_COMPUTE, 1},       // Command Buffer
        {RHI_PARAMETER_STORAGE_TEXEL, RHI_SHADER_STAGE_COMPUTE, 1}, // Count Buffer
    };

    static constexpr parameter_layout parameters = {
        {0, parameter},
        {1, scene},
    };
};

void cull_pass::add(render_graph& graph, const parameter& parameter)
{
    add_reset_pass(graph, parameter);

    rdg_buffer* cull_result = graph.add_buffer(
        "Cull Result",
        {
            .size = parameter.scene.get_mesh_capacity() * sizeof(std::uint32_t),
            .flags = RHI_BUFFER_STORAGE,
        });

    add_cull_pass(graph, parameter, cull_result);
    add_fill_pass(graph, parameter, cull_result);
}

void cull_pass::add_reset_pass(render_graph& graph, const parameter& parameter)
{
    auto& pass = graph.add_pass<rdg_pass>("Reset Count Buffer");
    pass.add_buffer(parameter.count_buffer, RHI_PIPELINE_STAGE_TRANSFER, RHI_ACCESS_TRANSFER_WRITE);
    pass.set_execute(
        [buffer = parameter.count_buffer](rdg_command& command)
        {
            command.fill_buffer(buffer->get_rhi(), {0, buffer->get_buffer_size()}, 0);
        });
}

void cull_pass::add_cull_pass(
    render_graph& graph,
    const parameter& parameter,
    rdg_buffer* cull_result)
{
    struct pass_data
    {
        rhi_parameter* cull_parameter;
        rhi_parameter* scene_parameter;
        rhi_parameter* camera_parameter;

        std::uint32_t mesh_count;
    };

    pass_data data = {
        .cull_parameter = graph.allocate_parameter(cull_cs::parameter),
        .scene_parameter = parameter.scene.get_scene_parameter(),
        .camera_parameter = parameter.camera.camera_parameter,
        .mesh_count = static_cast<std::uint32_t>(parameter.scene.get_mesh_count())};

    auto& cull_pass = graph.add_pass<rdg_compute_pass>("Cull Pass");
    cull_pass.add_buffer(cull_result, RHI_PIPELINE_STAGE_COMPUTE, RHI_ACCESS_SHADER_WRITE);
    cull_pass.set_execute(
        [data, cull_result](rdg_command& command)
        {
            data.cull_parameter->set_storage(0, cull_result->get_rhi());

            command.set_pipeline({
                .compute_shader = render_device::instance().get_shader<cull_cs>(),
            });
            command.set_parameter(0, data.cull_parameter);
            command.set_parameter(1, data.scene_parameter);
            command.set_parameter(2, data.camera_parameter);

            command.dispatch_1d(data.mesh_count);
        });
}

void cull_pass::add_fill_pass(
    render_graph& graph,
    const parameter& parameter,
    rdg_buffer* cull_result)
{
    struct pass_data
    {
        rdg_buffer* command_buffer;
        rdg_buffer* count_buffer;

        rhi_parameter* fill_parameter;
        rhi_parameter* scene_parameter;

        std::uint32_t instance_count;
    };

    pass_data data = {
        .command_buffer = parameter.command_buffer,
        .count_buffer = parameter.count_buffer,
        .fill_parameter = graph.allocate_parameter(draw_command_filler_cs::parameter),
        .scene_parameter = parameter.scene.get_scene_parameter(),
        .instance_count = static_cast<std::uint32_t>(parameter.scene.get_instance_count()),
    };

    data.fill_parameter->set_storage(0, parameter.scene.get_group_buffer());

    auto& fill_pass = graph.add_pass<rdg_compute_pass>("Fill Command Buffer");
    fill_pass.add_buffer(cull_result, RHI_PIPELINE_STAGE_COMPUTE, RHI_ACCESS_SHADER_READ);
    fill_pass.add_buffer(data.command_buffer, RHI_PIPELINE_STAGE_COMPUTE, RHI_ACCESS_SHADER_WRITE);
    fill_pass.add_buffer(data.count_buffer, RHI_PIPELINE_STAGE_COMPUTE, RHI_ACCESS_SHADER_WRITE);
    fill_pass.set_execute(
        [data, cull_result](rdg_command& command)
        {
            data.fill_parameter->set_storage(1, cull_result->get_rhi());
            data.fill_parameter->set_storage(2, data.command_buffer->get_rhi());
            data.fill_parameter->set_storage(3, data.count_buffer->get_rhi());

            command.set_pipeline({
                .compute_shader = render_device::instance().get_shader<draw_command_filler_cs>(),
            });
            command.set_parameter(0, data.fill_parameter);
            command.set_parameter(1, data.scene_parameter);
            command.dispatch_1d(data.instance_count);
        });
}
} // namespace violet