#include "graphics/passes/copy_depth_pass.hpp"

namespace violet
{
struct copy_depth_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/copy_depth.hlsl";

    struct copy_data
    {
        std::uint32_t src;
        std::uint32_t dst;
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(copy_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, parameter},
    };
};

void copy_depth_pass::add(render_graph& graph, const parameter& parameter)
{
    assert(
        parameter.src->get_extent().width == parameter.dst->get_extent().width &&
        parameter.src->get_extent().height == parameter.dst->get_extent().height);

    struct pass_data
    {
        rdg_texture_srv src;
        rdg_texture_uav dst;
        rhi_parameter* copy_parameter;
    };

    graph.add_pass<pass_data>(
        "Copy Depth Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.src = pass.add_texture_srv(parameter.src, RHI_PIPELINE_STAGE_COMPUTE);
            data.dst = pass.add_texture_uav(parameter.dst, RHI_PIPELINE_STAGE_COMPUTE);
            data.copy_parameter = pass.add_parameter(copy_depth_cs::parameter);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            copy_depth_cs::copy_data copy_data = {
                .src = data.src.get_bindless(),
                .dst = data.dst.get_bindless(),
            };

            data.copy_parameter->set_constant(0, &copy_data, sizeof(copy_depth_cs::copy_data));

            rdg_compute_pipeline pipeline = {
                .compute_shader = device.get_shader<copy_depth_cs>(),
            };
            command.set_pipeline(pipeline);
            command.set_parameter(0, device.get_bindless_parameter());
            command.set_parameter(1, data.copy_parameter);

            rhi_texture_extent extent = data.src.get_texture()->get_extent();

            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet