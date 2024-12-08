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

    static constexpr parameter copy = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(copy_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, copy},
    };
};

void copy_depth_pass::add(render_graph& graph, const parameter& parameter)
{
    assert(
        parameter.src->get_extent().width == parameter.dst->get_extent().width &&
        parameter.src->get_extent().height == parameter.dst->get_extent().height);

    struct pass_data
    {
        rdg_texture* src;
        rdg_texture* dst;

        rhi_parameter* copy_parameter;
    };

    pass_data data = {
        .src = parameter.src,
        .dst = parameter.dst,
        .copy_parameter = graph.allocate_parameter(copy_depth_cs::copy),
    };

    auto& pass = graph.add_pass<rdg_pass>("Copy Depth Pass");
    pass.add_texture(
        parameter.src,
        RHI_PIPELINE_STAGE_COMPUTE,
        RHI_ACCESS_SHADER_READ,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    pass.add_texture(
        parameter.dst,
        RHI_PIPELINE_STAGE_COMPUTE,
        RHI_ACCESS_SHADER_WRITE,
        RHI_TEXTURE_LAYOUT_GENERAL);
    pass.set_execute(
        [data](rdg_command& command)
        {
            copy_depth_cs::copy_data copy_constant = {
                .src = data.src->get_handle(),
                .dst = data.dst->get_handle(),
            };

            data.copy_parameter->set_constant(0, &copy_constant, sizeof(copy_depth_cs::copy_data));

            rdg_compute_pipeline pipeline = {
                .compute_shader = render_device::instance().get_shader<copy_depth_cs>(),
            };
            command.set_pipeline(pipeline);
            command.set_parameter(0, render_device::instance().get_bindless_parameter());
            command.set_parameter(1, data.copy_parameter);

            rhi_texture_extent extent = data.src->get_extent();

            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet