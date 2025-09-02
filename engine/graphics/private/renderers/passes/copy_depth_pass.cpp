#include "graphics/renderers/passes/copy_depth_pass.hpp"

namespace violet
{
struct copy_depth_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/copy_depth.hlsl";

    struct constant_data
    {
        std::uint32_t src;
        std::uint32_t dst;
        std::uint32_t width;
        std::uint32_t height;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

void copy_depth_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv src;
        rdg_texture_uav dst;
    };

    graph.add_pass<pass_data>(
        "Copy Depth Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.src = pass.add_texture_srv(parameter.src, RHI_PIPELINE_STAGE_COMPUTE);
            data.dst = pass.add_texture_uav(parameter.dst, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            rhi_texture_extent extent = data.src.get_extent();

            command.set_pipeline({
                .compute_shader = device.get_shader<copy_depth_cs>(),
            });

            command.set_constant(
                copy_depth_cs::constant_data{
                    .src = data.src.get_bindless(),
                    .dst = data.dst.get_bindless(),
                    .width = extent.width,
                    .height = extent.height,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet