#include "graphics/renderers/passes/motion_vector_pass.hpp"

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

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

void motion_vector_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv depth_buffer;
        rdg_texture_uav motion_vector;
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
        },
        [&](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            rhi_texture_extent extent = data.depth_buffer.get_texture()->get_extent();

            command.set_pipeline({
                .compute_shader = device.get_shader<motion_vector_cs>(),
            });
            command.set_constant(
                motion_vector_cs::constant_data{
                    .depth_buffer = data.depth_buffer.get_bindless(),
                    .motion_vector = data.motion_vector.get_bindless(),
                    .width = extent.width,
                    .height = extent.height,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_CAMERA);

            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet