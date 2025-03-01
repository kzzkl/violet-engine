#include "graphics/resources/brdf_lut.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
struct brdf_lut_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/brdf_lut.hlsl";

    struct constant_data
    {
        std::uint32_t brdf_lut;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t padding0;
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
    };
};

class brdf_lut_renderer
{
public:
    static void render(render_graph& graph, rhi_texture* output)
    {
        rdg_texture* brdf_lut = graph.add_texture(
            "BRDF LUT",
            output,
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        struct pass_data
        {
            rdg_texture_uav brdf_lut;
        };

        graph.add_pass<pass_data>(
            "BRDF LUT Pass",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.brdf_lut = pass.add_texture_uav(brdf_lut, RHI_PIPELINE_STAGE_COMPUTE);
            },
            [](const pass_data& data, rdg_command& command)
            {
                rhi_texture_extent extent = data.brdf_lut.get_texture()->get_extent();

                brdf_lut_cs::constant_data constant = {
                    .brdf_lut = data.brdf_lut.get_bindless(),
                    .width = extent.width,
                    .height = extent.height,
                };

                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<brdf_lut_cs>(),
                });
                command.set_constant(constant);
                command.set_parameter(0, render_device::instance().get_bindless_parameter());

                command.dispatch_2d(extent.width, extent.height);
            });
    }
};

brdf_lut::brdf_lut(std::uint32_t size)
{
    auto& device = render_device::instance();

    rhi_ptr<rhi_texture> brdf_lut = device.create_texture({
        .extent = {size, size},
        .format = RHI_FORMAT_R32G32_FLOAT,
        .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE,
    });

    render_graph graph("Generate BRDF LUT");

    brdf_lut_renderer::render(graph, brdf_lut.get());

    rhi_command* command = device.allocate_command();
    graph.compile();
    graph.record(command);
    device.execute_sync(command);

    set_texture(std::move(brdf_lut));
}
} // namespace violet