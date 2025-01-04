#include "graphics/passes/cull_pass.hpp"

namespace violet
{
struct cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/cull/cull.hlsl";

    struct cull_data
    {
        std::uint32_t cull_result;
        std::uint32_t padding0;
        std::uint32_t padding1;
        std::uint32_t padding2;
    };

    static constexpr parameter cull = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(cull_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, scene},
        {2, camera},
        {3, cull},
    };
};

struct draw_command_filler_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/cull/draw_command_filler.hlsl";

    struct fill_data
    {
        std::uint32_t cull_result;
        std::uint32_t command_buffer;
        std::uint32_t count_buffer;
        std::uint32_t padding0;
    };

    static constexpr parameter fill = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(fill_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, scene},
        {2, fill},
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
        rhi_parameter* scene_parameter;
        rhi_parameter* camera_parameter;
        rhi_parameter* cull_parameter;

        std::uint32_t mesh_count;
    };

    pass_data data = {
        .scene_parameter = parameter.scene.get_scene_parameter(),
        .camera_parameter = parameter.camera.camera_parameter,
        .cull_parameter = graph.allocate_parameter(cull_cs::cull),
        .mesh_count = static_cast<std::uint32_t>(parameter.scene.get_mesh_count()),
    };

    auto& cull_pass = graph.add_pass<rdg_compute_pass>("Cull Pass");
    cull_pass.add_buffer(cull_result, RHI_PIPELINE_STAGE_COMPUTE, RHI_ACCESS_SHADER_WRITE);
    cull_pass.set_execute(
        [data, cull_result](rdg_command& command)
        {
            cull_cs::cull_data cull_data = {
                .cull_result = cull_result->get_handle(),
            };
            data.cull_parameter->set_constant(0, &cull_data, sizeof(cull_cs::cull_data));

            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<cull_cs>(),
            });
            command.set_parameter(0, device.get_bindless_parameter());
            command.set_parameter(1, data.scene_parameter);
            command.set_parameter(2, data.camera_parameter);
            command.set_parameter(3, data.cull_parameter);

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
        rhi_parameter* scene_parameter;
        rhi_parameter* fill_parameter;

        rdg_buffer* command_buffer;
        rdg_buffer* count_buffer;

        std::uint32_t instance_count;
    };

    pass_data data = {
        .scene_parameter = parameter.scene.get_scene_parameter(),
        .fill_parameter = graph.allocate_parameter(draw_command_filler_cs::fill),
        .command_buffer = parameter.command_buffer,
        .count_buffer = parameter.count_buffer,
        .instance_count = static_cast<std::uint32_t>(parameter.scene.get_instance_count()),
    };

    auto& fill_pass = graph.add_pass<rdg_compute_pass>("Fill Command Buffer");
    fill_pass.add_buffer(cull_result, RHI_PIPELINE_STAGE_COMPUTE, RHI_ACCESS_SHADER_READ);
    fill_pass.add_buffer(data.command_buffer, RHI_PIPELINE_STAGE_COMPUTE, RHI_ACCESS_SHADER_WRITE);
    fill_pass.add_buffer(data.count_buffer, RHI_PIPELINE_STAGE_COMPUTE, RHI_ACCESS_SHADER_WRITE);
    fill_pass.set_execute(
        [data, cull_result](rdg_command& command)
        {
            draw_command_filler_cs::fill_data fill_data = {
                .cull_result = cull_result->get_handle(),
                .command_buffer = data.command_buffer->get_handle(),
                .count_buffer = data.count_buffer->get_handle(),
            };
            data.fill_parameter->set_constant(
                0,
                &fill_data,
                sizeof(draw_command_filler_cs::fill_data));

            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<draw_command_filler_cs>(),
            });
            command.set_parameter(0, device.get_bindless_parameter());
            command.set_parameter(1, data.scene_parameter);
            command.set_parameter(2, data.fill_parameter);
            command.dispatch_1d(data.instance_count);
        });
}
} // namespace violet