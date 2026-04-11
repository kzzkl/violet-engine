#include "graphics/renderers/passes/dithering_pass.hpp"
#include "graphics/resources/spatiotemporal_blue_noise.hpp"

namespace violet
{
struct dithering_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/dithering.hlsl";

    struct constant_data
    {
        vec2u size;
        vec2f size_inv;
        std::uint32_t render_target;
        std::uint32_t blue_noise;
        vec2f blue_noise_size_inv;
        std::uint32_t blue_noise_slice;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

void dithering_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_uav render_target;
        std::uint32_t frame;
    };

    graph.add_pass<pass_data>(
        "Dithering Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.render_target =
                pass.add_texture_uav(parameter.render_target, RHI_PIPELINE_STAGE_COMPUTE);
            data.frame = parameter.frame;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<dithering_cs>(),
            });

            auto extent = data.render_target.get_extent();

            auto* blue_noise_texture = device.get_texture<spatiotemporal_blue_noise>();
            rhi_extent blue_noise_extent = blue_noise_texture->get_rhi()->get_extent();
            std::uint32_t blue_noise_layer_count = blue_noise_texture->get_rhi()->get_layer_count();

            command.set_constant(
                dithering_cs::constant_data{
                    .size = vec2u(extent.width, extent.height),
                    .size_inv = vec2f(
                        1.0f / static_cast<float>(extent.width),
                        1.0f / static_cast<float>(extent.height)),
                    .render_target = data.render_target.get_bindless(),
                    .blue_noise =
                        blue_noise_texture->get_srv(RHI_TEXTURE_DIMENSION_2D_ARRAY)->get_bindless(),
                    .blue_noise_size_inv = vec2f(
                        1.0f / static_cast<float>(blue_noise_extent.width),
                        1.0f / static_cast<float>(blue_noise_extent.height)),
                    .blue_noise_slice = data.frame % blue_noise_layer_count,
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet