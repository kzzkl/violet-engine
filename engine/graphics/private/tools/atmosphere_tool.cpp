#include "tools/atmosphere_tool.hpp"
#include "graphics/render_device.hpp"
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
        float mie_absorption;
        float mie_density_height;

        float ozone_center_height;
        vec3f ozone_absorption;
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

void atmosphere_tool::generate_transmittance_lut(
    vec3f rayleigh_scattering,
    float rayleigh_density_height,
    float mie_scattering,
    float mie_absorption,
    float mie_density_height,
    float ozone_center_height,
    vec3f ozone_absorption,
    float ozone_width,
    float planet_radius,
    float atmosphere_height,
    std::uint32_t sample_count,
    rhi_texture* transmittance_lut,
    rhi_command* command)
{
    render_graph graph("Generate Transmittance LUT");

    rdg_texture* output = graph.add_texture(
        "Transmittance LUT",
        transmittance_lut,
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
            data.transmittance_lut = pass.add_texture_uav(output, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [=](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<transmittance_lut_cs>(),
            });

            command.set_constant(
                transmittance_lut_cs::constant_data{
                    .rayleigh_scattering = rayleigh_scattering,
                    .rayleigh_density_height = rayleigh_density_height,
                    .mie_scattering = mie_scattering,
                    .mie_absorption = mie_absorption,
                    .mie_density_height = mie_density_height,
                    .ozone_center_height = ozone_center_height,
                    .ozone_absorption = ozone_absorption,
                    .ozone_width = ozone_width,
                    .planet_radius = planet_radius,
                    .atmosphere_height = atmosphere_height,
                    .transmittance_lut = data.transmittance_lut.get_bindless(),
                    .sample_count = sample_count,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            rhi_extent extent = data.transmittance_lut.get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });

    graph.compile();

    if (command == nullptr)
    {
        auto& device = render_device::instance();
        command = device.allocate_command();
        graph.record(command);
        device.execute_sync(command);
    }
    else
    {
        graph.record(command);
    }
}
} // namespace violet
