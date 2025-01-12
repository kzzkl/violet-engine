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

void motion_vector_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture* depth_buffer;
        rdg_texture* motion_vector;

        rhi_parameter* camera_parameter;
        rhi_parameter* constant_parameter;
    };

    pass_data data = {
        .depth_buffer = parameter.depth_buffer,
        .motion_vector = parameter.motion_vector,
        .camera_parameter = parameter.camera.camera_parameter,
        .constant_parameter = graph.allocate_parameter(motion_vector_cs::parameter),
    };

    auto& pass = graph.add_pass<rdg_compute_pass>("Motion Vector Pass");
    pass.add_texture(
        data.depth_buffer,
        RHI_PIPELINE_STAGE_COMPUTE,
        RHI_ACCESS_SHADER_READ,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    pass.add_texture(
        data.motion_vector,
        RHI_PIPELINE_STAGE_COMPUTE,
        RHI_ACCESS_SHADER_WRITE,
        RHI_TEXTURE_LAYOUT_GENERAL);
    pass.set_execute(
        [data](rdg_command& command)
        {
            auto& device = render_device::instance();

            rhi_texture_extent extent = data.depth_buffer->get_extent();

            motion_vector_cs::constant_data constant_data = {
                .depth_buffer = data.depth_buffer->get_handle(),
                .motion_vector = data.motion_vector->get_handle(),
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
            command.set_parameter(1, data.camera_parameter);
            command.set_parameter(2, data.constant_parameter);

            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet