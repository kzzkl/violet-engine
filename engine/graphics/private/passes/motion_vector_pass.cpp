#include "graphics/passes/motion_vector_pass.hpp"

namespace violet
{
struct motion_vector_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/motion_vector.hlsl";

    struct constant_data
    {
        std::uint32_t depth_buffer;
        std::uint32_t motion_vector;

        std::uint32_t width;
        std::uint32_t height;
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(constant_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, camera},
        {2, parameter},
    };
};

void motion_vector_pass::add(
    render_graph& graph,
    const render_context& context,
    const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv depth_buffer;
        rdg_texture_uav motion_vector;

        rhi_parameter* constant_parameter;
    };

    graph.add_pass<pass_data>(
        "Motion Vector Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.depth_buffer =
                pass.add_texture_srv(parameter.depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.motion_vector =
                pass.add_texture_uav(parameter.motion_vector, RHI_PIPELINE_STAGE_COMPUTE);
            data.constant_parameter = pass.add_parameter(motion_vector_cs::parameter);
        },
        [&](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            rhi_texture_extent extent = data.depth_buffer.get_texture()->get_extent();

            motion_vector_cs::constant_data constant_data = {
                .depth_buffer = data.depth_buffer.get_bindless(),
                .motion_vector = data.motion_vector.get_bindless(),
                .width = extent.width,
                .height = extent.height,
            };

            data.constant_parameter->set_constant(
                0,
                &constant_data,
                sizeof(motion_vector_cs::constant_data));

            command.set_pipeline({
                .compute_shader = device.get_shader<motion_vector_cs>(),
            });

            command.set_parameter(0, device.get_bindless_parameter());
            command.set_parameter(1, context.get_camera_parameter());
            command.set_parameter(2, data.constant_parameter);

            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet