#include "graphics/passes/cull_pass.hpp"

namespace violet
{
struct cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/cull/instance_cull.hlsl";

    struct constant_data
    {
        std::uint32_t hzb_buffer;
        std::uint32_t cull_result;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t frustum_culling;
        std::uint32_t occlusion_culling;
        std::uint32_t padding0;
        std::uint32_t padding1;
        vec4f frustum;
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, scene},
        {2, camera},
    };
};

struct draw_command_filler_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/cull/draw_command_filler.hlsl";

    struct constant_data
    {
        std::uint32_t cull_result;
        std::uint32_t command_buffer;
        std::uint32_t count_buffer;
        std::uint32_t padding0;
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, scene},
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
        rdg_texture_srv hzb_buffer;
        rdg_buffer_uav cull_result;
        bool frustum_culling;
        bool occlusion_culling;
        std::uint32_t mesh_count;

        vec4f frustum;
    };

    graph.add_pass<pass_data>(
        "Cull Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.hzb_buffer = pass.add_texture_srv(parameter.hzb, RHI_PIPELINE_STAGE_COMPUTE);
            data.cull_result = pass.add_buffer_uav(cull_result, RHI_PIPELINE_STAGE_COMPUTE);
            data.frustum_culling = parameter.frustum_culling;
            data.occlusion_culling = parameter.occlusion_culling;
            data.mesh_count = graph.get_scene().get_mesh_count();

            mat4f matrix_p_t = matrix::transpose(graph.get_camera().get_matrix_p());
            vec4f frustum_x = vector::normalize(matrix_p_t[3] + matrix_p_t[0]);
            frustum_x /= vector::length(vec3f(frustum_x));
            vec4f frustum_y = vector::normalize(matrix_p_t[3] + matrix_p_t[0]);
            frustum_y /= vector::length(vec3f(frustum_y));

            data.frustum[0] = frustum_x.x;
            data.frustum[1] = frustum_x.z;
            data.frustum[2] = frustum_y.y;
            data.frustum[3] = frustum_y.z;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<cull_cs>(),
            });

            rhi_texture_extent extent = data.hzb_buffer.get_extent();

            command.set_constant(cull_cs::constant_data{
                .hzb_buffer = data.hzb_buffer.get_bindless(),
                .cull_result = data.cull_result.get_bindless(),
                .width = extent.width,
                .height = extent.height,
                .frustum_culling = data.frustum_culling,
                .occlusion_culling = data.occlusion_culling,
                .frustum = data.frustum,
            });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);

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
            data.instance_count = graph.get_scene().get_instance_count();
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<draw_command_filler_cs>(),
            });
            command.set_constant(draw_command_filler_cs::constant_data{
                .cull_result = data.cull_result.get_bindless(),
                .command_buffer = data.command_buffer.get_bindless(),
                .count_buffer = data.count_buffer.get_bindless(),
            });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.dispatch_1d(data.instance_count);
        });
}
} // namespace violet