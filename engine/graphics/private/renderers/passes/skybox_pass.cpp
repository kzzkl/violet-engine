#include "graphics/renderers/passes/skybox_pass.hpp"
#include "graphics/render_interface.hpp"
#include "graphics/resources/transmittance_lut.hpp"
#include "graphics/shader.hpp"

namespace violet
{
struct skybox_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/skybox.hlsl";

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 2, .desc = camera},
    };
};

struct skybox_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/skybox.hlsl";

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
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
    };
};

void skybox_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        bool use_atmospheric_scattering;
    };

    atmosphere atmosphere = graph.get_scene().get_atmosphere();

    graph.add_pass<pass_data>(
        "Skybox Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            rhi_attachment_load_op load_op =
                parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD;

            pass.add_render_target(parameter.render_target, load_op);
            pass.set_depth_stencil(parameter.depth_buffer, load_op);

            data.use_atmospheric_scattering = parameter.use_atmospheric_scattering;
        },
        [atmosphere](const pass_data& data, rdg_command& command)
        {
            command.set_viewport();
            command.set_scissor();

            auto& device = render_device::instance();

            std::vector<std::wstring> defines;
            if (data.use_atmospheric_scattering)
            {
                defines.emplace_back(L"-DUSE_ATMOSPHERIC_SCATTERING");
            }

            rdg_raster_pipeline pipeline = {
                .vertex_shader = device.get_shader<skybox_vs>(defines),
                .fragment_shader = device.get_shader<skybox_fs>(defines),
                .depth_stencil_state =
                    device.get_depth_stencil_state<true, false, RHI_COMPARE_OP_EQUAL>(),
            };

            command.set_pipeline(pipeline);

            command.set_constant(
                skybox_fs::constant_data{
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
                    .transmittance_lut =
                        device.get_buildin_texture<transmittance_lut>()->get_srv()->get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);

            command.draw(0, 36);
        });
}
} // namespace violet