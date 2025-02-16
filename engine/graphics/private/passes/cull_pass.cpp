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

    static constexpr parameter parameter = {
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
        {3, parameter},
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

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(fill_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, scene},
        {2, parameter},
    };
};

void cull_pass::add(render_graph& graph, const parameter& parameter)
{
    add_reset_pass(graph, parameter);

    rdg_buffer* cull_result = graph.add_buffer(
        "Cull Result",
        graph.get_scene().get_mesh_capacity() * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE);

    add_cull_pass(graph, parameter, cull_result);
    add_fill_pass(graph, parameter, cull_result);
}

void cull_pass::add_reset_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_buffer_ref count_buffer;
    };

    graph.add_pass<pass_data>(
        "Reset Count Buffer",
        RDG_PASS_TRANSFER,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.count_buffer = pass.add_buffer(
                parameter.count_buffer,
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_ACCESS_TRANSFER_WRITE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            command.fill_buffer(data.count_buffer.get_rhi(), {0, data.count_buffer.get_size()}, 0);
        });
}

void cull_pass::add_cull_pass(
    render_graph& graph,
    const parameter& parameter,
    rdg_buffer* cull_result)
{
    struct pass_data
    {
        rdg_buffer_uav cull_result;
        rhi_parameter* cull_parameter;

        std::uint32_t mesh_count;
    };

    graph.add_pass<pass_data>(
        "Cull Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.cull_result = pass.add_buffer_uav(cull_result, RHI_PIPELINE_STAGE_COMPUTE);
            data.cull_parameter = pass.add_parameter(cull_cs::parameter);
            data.mesh_count = graph.get_scene().get_mesh_count();
        },
        [&](const pass_data& data, rdg_command& command)
        {
            cull_cs::cull_data cull_data = {
                .cull_result = data.cull_result.get_bindless(),
            };
            data.cull_parameter->set_constant(0, &cull_data, sizeof(cull_cs::cull_data));

            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<cull_cs>(),
            });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);
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
        rdg_buffer_srv cull_result;
        rdg_buffer_uav command_buffer;
        rdg_buffer_uav count_buffer;

        rhi_parameter* fill_parameter;

        std::uint32_t instance_count;
    };

    graph.add_pass<pass_data>(
        "Fill Command Buffer",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.cull_result = pass.add_buffer_srv(cull_result, RHI_PIPELINE_STAGE_COMPUTE);
            data.command_buffer =
                pass.add_buffer_uav(parameter.command_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.count_buffer = pass.add_buffer_uav(
                parameter.count_buffer,
                RHI_PIPELINE_STAGE_COMPUTE,
                0,
                0,
                RHI_FORMAT_R32_FLOAT);
            data.fill_parameter = pass.add_parameter(draw_command_filler_cs::parameter);
            data.instance_count = graph.get_scene().get_instance_count();
        },
        [](const pass_data& data, rdg_command& command)
        {
            draw_command_filler_cs::fill_data fill_data = {
                .cull_result = data.cull_result.get_bindless(),
                .command_buffer = data.command_buffer.get_bindless(),
                .count_buffer = data.count_buffer.get_bindless(),
            };
            data.fill_parameter->set_constant(
                0,
                &fill_data,
                sizeof(draw_command_filler_cs::fill_data));

            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<draw_command_filler_cs>(),
            });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, data.fill_parameter);
            command.dispatch_1d(data.instance_count);
        });
}
} // namespace violet