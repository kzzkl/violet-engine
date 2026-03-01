#include "graphics/resources/transmittance_lut.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
struct transmittance_lut_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/transmittance_lut.hlsl";

    struct constant_data
    {
        vec3f rayleigh_scattering;
        float rayleigh_density_height;

        float mie_scattering;
        float mie_asymmetry;
        float mie_absorption;
        float mie_density_height;

        vec3f ozone_absorption;
        float ozone_center_height;
        float ozone_width;

        float planet_radius;
        float atmosphere_height;

        std::uint32_t transmittance_lut;
        std::uint32_t sample_count;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

class transmittance_lut_renderer
{
public:
    static void render(render_graph& graph, rhi_texture* output, const atmosphere& atmosphere)
    {
        rdg_texture* transmittance_lut = graph.add_texture(
            "Transmittance LUT",
            output,
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        struct pass_data
        {
            rdg_texture_uav transmittance_lut;
        };

        graph.add_pass<pass_data>(
            "Transmittance LUT",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.transmittance_lut =
                    pass.add_texture_uav(transmittance_lut, RHI_PIPELINE_STAGE_COMPUTE);
            },
            [atmosphere](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<transmittance_lut_cs>(),
                });

                command.set_constant(
                    transmittance_lut_cs::constant_data{
                        .rayleigh_scattering = atmosphere.rayleigh_scattering,
                        .rayleigh_density_height = atmosphere.rayleigh_density_height,
                        .mie_scattering = atmosphere.mie_scattering,
                        .mie_asymmetry = atmosphere.mie_asymmetry,
                        .mie_absorption = atmosphere.mie_absorption,
                        .mie_density_height = atmosphere.mie_density_height,
                        .ozone_absorption = atmosphere.ozone_absorption,
                        .ozone_center_height = atmosphere.ozone_center_height,
                        .ozone_width = atmosphere.ozone_width,
                        .planet_radius = atmosphere.planet_radius,
                        .atmosphere_height = atmosphere.atmosphere_height,
                        .transmittance_lut = data.transmittance_lut.get_bindless(),
                        .sample_count = 1024,
                    });
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                rhi_texture_extent extent = data.transmittance_lut.get_extent();
                command.dispatch_2d(extent.width, extent.height);
            });
    }
};

transmittance_lut::transmittance_lut(const atmosphere& atmosphere)
{
    auto& device = render_device::instance();

    rhi_ptr<rhi_texture> transmittance_lut = device.create_texture({
        .extent =
            {
                .width = 256,
                .height = 256,
            },
        .format = RHI_FORMAT_R11G11B10_FLOAT,
        .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE,
        .level_count = 1,
        .layer_count = 1,
    });
    transmittance_lut->set_name("Transmittance LUT");

    render_graph graph("Generate Transmittance LUT");

    transmittance_lut_renderer::render(graph, transmittance_lut.get(), atmosphere);

    rhi_command* command = device.allocate_command();
    graph.compile();
    graph.record(command);
    device.execute_sync(command);

    set_texture(std::move(transmittance_lut));
}
} // namespace violet