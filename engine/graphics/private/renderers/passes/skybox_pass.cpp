#include "graphics/renderers/passes/skybox_pass.hpp"
#include "graphics/render_interface.hpp"
#include "graphics/shader.hpp"
#include "graphics/skybox.hpp"

namespace violet
{
struct sky_view_lut_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/sky_view_lut.hlsl";

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
        float atmosphere_radius;

        std::uint32_t sky_view_lut;
        vec3f sun_direction;
        std::uint32_t transmittance_lut;
        vec3f sun_irradiance;
        std::uint32_t sample_count;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

struct sky_constant_data
{
    vec3f sun_direction;
    float sun_angular_radius;
    vec3f sun_irradiance;
    float planet_radius;
    float atmosphere_radius;
    std::uint32_t skybox_texture;
    std::uint32_t sky_view_lut;
    std::uint32_t transmittance_lut;
};

struct sky_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/sky.hlsl";

    using constant_data = sky_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 2, .desc = camera},
    };
};

struct sky_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/sky.hlsl";

    using constant_data = sky_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
    };
};

void skybox_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Skybox Pass");

    m_skybox = graph.get_scene().get_skybox();

    if (m_skybox->is_dynamic_sky())
    {
        add_sky_view_lut_pass(graph, parameter);
    }

    add_sky_pass(graph, parameter);
}

void skybox_pass::add_sky_view_lut_pass(render_graph& graph, const parameter& parameter)
{
    m_sky_view_lut = graph.add_texture(
        "Sky View LUT",
        {.width = 192, .height = 108},
        RHI_FORMAT_R11G11B10_FLOAT,
        RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE);

    struct pass_data
    {
        vec3f sun_direction;
        vec3f sun_irradiance;
        rdg_texture_uav sky_view_lut;

        skybox* skybox;
    };

    graph.add_pass<pass_data>(
        "Sky View LUT Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.sky_view_lut = pass.add_texture_uav(m_sky_view_lut, RHI_PIPELINE_STAGE_COMPUTE);
            data.sun_direction = graph.get_scene().get_sun_direction();
            data.sun_irradiance = graph.get_scene().get_sun_irradiance();
            data.skybox = graph.get_scene().get_skybox();
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<sky_view_lut_cs>(),
            });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_CAMERA);

            command.set_constant(
                sky_view_lut_cs::constant_data{
                    .rayleigh_scattering = data.skybox->get_rayleigh_scattering(),
                    .rayleigh_density_height = data.skybox->get_rayleigh_density_height(),
                    .mie_scattering = data.skybox->get_mie_scattering(),
                    .mie_asymmetry = data.skybox->get_mie_asymmetry(),
                    .mie_absorption = data.skybox->get_mie_absorption(),
                    .mie_density_height = data.skybox->get_mie_density_height(),
                    .ozone_absorption = data.skybox->get_ozone_absorption(),
                    .ozone_center_height = data.skybox->get_ozone_center_height(),
                    .ozone_width = data.skybox->get_ozone_width(),
                    .planet_radius = data.skybox->get_planet_radius(),
                    .atmosphere_radius =
                        data.skybox->get_planet_radius() + data.skybox->get_atmosphere_height(),
                    .sky_view_lut = data.sky_view_lut.get_bindless(),
                    .sun_direction = data.sun_direction,
                    .transmittance_lut =
                        data.skybox->get_transmittance_lut()->get_srv()->get_bindless(),
                    .sun_irradiance = data.sun_irradiance,
                    .sample_count = 40,
                });

            auto extent = data.sky_view_lut.get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });
}

void skybox_pass::add_aerial_lut_pass(render_graph& graph, const parameter& parameter) {
    
}

void skybox_pass::add_sky_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv sky_view_lut;
        skybox* skybox;

        vec3f sun_direction;
        vec3f sun_irradiance;
    };

    graph.add_pass<pass_data>(
        "Skybox Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            rhi_attachment_load_op load_op =
                parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD;

            pass.add_render_target(parameter.render_target, load_op);
            pass.set_depth_stencil(parameter.depth_buffer, load_op);

            if (m_skybox->is_dynamic_sky())
            {
                data.sky_view_lut =
                    pass.add_texture_srv(m_sky_view_lut, RHI_PIPELINE_STAGE_FRAGMENT);
            }

            data.skybox = m_skybox;
            data.sun_direction = graph.get_scene().get_sun_direction();
            data.sun_irradiance = graph.get_scene().get_sun_irradiance();
        },
        [](const pass_data& data, rdg_command& command)
        {
            command.set_viewport();
            command.set_scissor();

            auto& device = render_device::instance();

            std::vector<std::wstring> defines;
            if (data.skybox->is_dynamic_sky())
            {
                defines.emplace_back(L"-DDYNAMIC_SKY");
            }

            rdg_raster_pipeline pipeline = {
                .vertex_shader = device.get_shader<sky_vs>(defines),
                .fragment_shader = device.get_shader<sky_fs>(defines),
                .depth_stencil_state =
                    device.get_depth_stencil_state<true, false, RHI_COMPARE_OP_EQUAL>(),
            };

            command.set_pipeline(pipeline);

            command.set_constant(
                sky_constant_data{
                    .sun_direction = data.sun_direction,
                    .sun_angular_radius = data.skybox->get_sun_angular_radius(),
                    .sun_irradiance = data.sun_irradiance,
                    .planet_radius = data.skybox->get_planet_radius(),
                    .atmosphere_radius = data.skybox->get_atmosphere_radius(),
                    .skybox_texture = data.skybox->get_skybox_texture_bindless(),
                    .sky_view_lut = data.sky_view_lut.get_bindless(),
                    .transmittance_lut = data.skybox->get_transmittance_lut_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);

            command.draw(0, 36);
        });
}
} // namespace violet