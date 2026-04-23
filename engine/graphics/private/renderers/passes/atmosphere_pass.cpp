#include "graphics/renderers/passes/atmosphere_pass.hpp"
#include "graphics/renderers/passes/ibl_pass.hpp"

namespace violet
{
namespace
{
vec2f get_transmittance_lut_uv(float r, float mu, float planet_radius, float atmosphere_radius)
{
    float h = std::sqrt((atmosphere_radius * atmosphere_radius) - (planet_radius * planet_radius));
    float rho = std::sqrt((r * r) - (planet_radius * planet_radius));

    float discriminant = (r * r * (mu * mu - 1.0f)) + (atmosphere_radius * atmosphere_radius);
    float d = std::max(0.0f, ((-r * mu) + std::sqrt(discriminant)));

    float d_min = atmosphere_radius - r;
    float d_max = rho + h;

    float x_mu = (d - d_min) / (d_max - d_min);
    float x_r = rho / h;

    return {x_mu, x_r};
}

vec2f get_transmittance_lut_uv(
    const vec3f& eye,
    const vec3f& sun_direction,
    float planet_radius,
    float atmosphere_radius)
{
    float r = std::max(eye.y + planet_radius, planet_radius + 1.0f);
    float mu = -sun_direction.y;

    return get_transmittance_lut_uv(r, mu, planet_radius, atmosphere_radius);
}
} // namespace

static constexpr float AERIAL_PERSPECTIVE_SLICE_DISTANCE = 2000.0f;

struct sky_view_lut_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/sky_view_lut.hlsl";

    struct constant_data
    {
        atmosphere_data atmosphere;

        vec3f sun_direction;
        std::uint32_t sky_view_lut;
        vec3f sun_irradiance;
        std::uint32_t sample_count;
        vec3f ground_color;
        std::uint32_t transmittance_lut;
        std::uint32_t multi_scattering_lut;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

struct environment_map_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/environment_map.hlsl";

    struct constant_data
    {
        std::uint32_t sky_view_lut;
        std::uint32_t environment_map;
        std::uint32_t width;
        std::uint32_t height;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct aerial_perspective_lut_cs : public shader_cs
{
    static constexpr std::string_view path =
        "assets/shaders/atmosphere/aerial_perspective_lut.hlsl";

    struct constant_data
    {
        atmosphere_data atmosphere;

        vec3f sun_direction;
        float distance_per_slice;
        vec3f sun_irradiance;
        std::uint32_t sample_count;
        vec3f frustum_top_left;
        std::uint32_t transmittance_lut;
        vec3f frustum_top_right;
        std::uint32_t aerial_perspective_lut;
        vec3f frustum_bottom_left;
        std::uint32_t multi_scattering_lut;
        vec3f frustum_bottom_right;
        std::uint32_t vsm_id;
        std::uint32_t vsm_buffer;
        std::uint32_t vsm_virtual_page_table;
        std::uint32_t vsm_physical_shadow_map;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
        {.space = 2, .desc = camera},
    };
};

struct aerial_perspective_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/aerial_perspective.hlsl";

    struct constant_data
    {
        std::uint32_t render_target;
        std::uint32_t depth_buffer;
        std::uint32_t aerial_perspective_lut;
        float distance_per_slice;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

struct atmosphere_constant_data
{
    vec3f sun_direction;
    float sun_angular_radius;
    vec3f sun_irradiance;
    float planet_radius;
    float atmosphere_radius;
    std::uint32_t skybox_texture;
    std::uint32_t sky_view_lut;
    std::uint32_t transmittance_lut;
    vec2f transmittance_lut_uv;
    std::uint32_t sky_view_lut_sampler;
};

struct atmosphere_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/sky.hlsl";

    using constant_data = atmosphere_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 2, .desc = camera},
    };
};

struct atmosphere_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/sky.hlsl";

    using constant_data = atmosphere_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
    };
};

void atmosphere_lut_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Atmosphere LUT");

    add_sky_view_lut_pass(graph, parameter);
    add_aerial_perspective_lut_pass(graph, parameter);

    if (parameter.prefilter_map != nullptr && parameter.irradiance_sh != nullptr)
    {
        m_environment_map = graph.add_texture(
            "Environment Map",
            {.width = 128, .height = 128},
            RHI_FORMAT_R11G11B10_FLOAT,
            RHI_TEXTURE_CUBE | RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE,
            1,
            6);

        add_environment_map_pass(graph, parameter);

        graph.add_pass<prefilter_pass>({
            .environment_map = m_environment_map,
            .prefilter_map = parameter.prefilter_map,
        });

        graph.add_pass<irradiance_pass>({
            .environment_map = m_environment_map,
            .irradiance_sh = parameter.irradiance_sh,
        });
    }
}

void atmosphere_lut_pass::add_sky_view_lut_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_uav sky_view_lut;
        atmosphere_data atmosphere;
        vec3f sun_direction;
        vec3f sun_irradiance;
        vec3f ground_color;

        rhi_texture_srv* transmittance_lut;
        rhi_texture_srv* multi_scattering_lut;
    };

    graph.add_pass<pass_data>(
        "Sky View LUT Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            const auto& context = graph.get_context();

            data.sky_view_lut =
                pass.add_texture_uav(parameter.sky_view_lut, RHI_PIPELINE_STAGE_COMPUTE);
            data.atmosphere = context.get_atmosphere();
            data.sun_direction = context.get_sun_direction();
            data.sun_irradiance = context.get_sun_irradiance();
            data.ground_color = context.get_atmosphere().ground_color;
            data.transmittance_lut = context.get_transmittance_lut()->get_srv();
            data.multi_scattering_lut = parameter.use_multi_scattering ?
                                            context.get_multi_scattering_lut()->get_srv() :
                                            nullptr;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            std::vector<std::wstring> defines;

            sky_view_lut_cs::constant_data constant = {
                .atmosphere = data.atmosphere,
                .sun_direction = data.sun_direction,
                .sky_view_lut = data.sky_view_lut.get_bindless(),
                .sun_irradiance = data.sun_irradiance,
                .sample_count = 40,
                .ground_color = data.ground_color,
                .transmittance_lut = data.transmittance_lut->get_bindless(),
            };

            if (data.multi_scattering_lut != nullptr)
            {
                defines.emplace_back(L"-DUSE_MULTI_SCATTERING");
                constant.multi_scattering_lut = data.multi_scattering_lut->get_bindless();
            }

            command.set_pipeline({
                .compute_shader = device.get_shader<sky_view_lut_cs>(defines),
            });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_CAMERA);

            command.set_constant(constant);

            auto extent = data.sky_view_lut.get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });
}

void atmosphere_lut_pass::add_aerial_perspective_lut_pass(
    render_graph& graph,
    const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_uav aerial_perspective_lut;

        atmosphere_data atmosphere;
        vec3f sun_direction;
        vec3f sun_irradiance;

        rhi_texture_srv* transmittance_lut;
        rhi_texture_srv* multi_scattering_lut;

        std::uint32_t vsm_id;
        rdg_buffer_srv vsm_buffer;
        rdg_buffer_srv vsm_virtual_page_table;
        rdg_texture_srv vsm_physical_shadow_map;
    };

    const auto& context = graph.get_context();

    mat4f matrix_p = context.get_camera_matrix_p();
    mat4f matrix_v_inv = matrix::inverse(context.get_camera_matrix_v());

    auto get_corner_direction = [&](const vec2f& ndc)
    {
        vec4f direction =
            vector::normalize(vec4f(ndc.x / matrix_p[0][0], ndc.y / matrix_p[1][1], 1.0f, 0.0));
        return vec3f(matrix::mul(direction, matrix_v_inv));
    };

    vec3f frustum_top_left = get_corner_direction({-1.0f, 1.0f});
    vec3f frustum_top_right = get_corner_direction({1.0f, 1.0f});
    vec3f frustum_bottom_left = get_corner_direction({-1.0f, -1.0f});
    vec3f frustum_bottom_right = get_corner_direction({1.0f, -1.0f});

    graph.add_pass<pass_data>(
        "Aerial Perspective LUT Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.aerial_perspective_lut = pass.add_texture_uav(
                parameter.aerial_perspective_lut,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_TEXTURE_DIMENSION_3D);

            data.atmosphere = context.get_atmosphere();
            data.sun_direction = context.get_sun_direction();
            data.sun_irradiance = context.get_sun_irradiance();
            data.transmittance_lut = context.get_transmittance_lut()->get_srv();

            if (parameter.use_multi_scattering)
            {
                data.multi_scattering_lut = context.get_multi_scattering_lut()->get_srv();
            }
            else
            {
                data.multi_scattering_lut = nullptr;
            }

            bool cast_shadow = false;
            std::uint32_t sun_index = context.get_sun_index(cast_shadow);

            if (cast_shadow && parameter.vsm_buffer != nullptr)
            {
                data.vsm_id = context.get_vsm_id(sun_index);
                data.vsm_buffer =
                    pass.add_buffer_srv(parameter.vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                data.vsm_virtual_page_table = pass.add_buffer_srv(
                    parameter.vsm_virtual_page_table,
                    RHI_PIPELINE_STAGE_COMPUTE);
                data.vsm_physical_shadow_map = pass.add_texture_srv(
                    parameter.vsm_physical_shadow_map,
                    RHI_PIPELINE_STAGE_COMPUTE);
            }
            else
            {
                data.vsm_id = 0xFFFFFFFF;
            }
        },
        [=](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            std::vector<std::wstring> defines;

            aerial_perspective_lut_cs::constant_data constant = {
                .atmosphere = data.atmosphere,
                .sun_direction = data.sun_direction,
                .distance_per_slice = AERIAL_PERSPECTIVE_SLICE_DISTANCE,
                .sun_irradiance = data.sun_irradiance,
                .sample_count = 40,
                .frustum_top_left = frustum_top_left,
                .transmittance_lut = data.transmittance_lut->get_bindless(),
                .frustum_top_right = frustum_top_right,
                .aerial_perspective_lut = data.aerial_perspective_lut.get_bindless(),
                .frustum_bottom_left = frustum_bottom_left,
                .frustum_bottom_right = frustum_bottom_right,
            };

            if (data.multi_scattering_lut != nullptr)
            {
                defines.emplace_back(L"-DUSE_MULTI_SCATTERING");
                constant.multi_scattering_lut = data.multi_scattering_lut->get_bindless();
            }

            if (data.vsm_id != 0xFFFFFFFF)
            {
                defines.emplace_back(L"-DUSE_SHADOW");
                constant.vsm_id = data.vsm_id;
                constant.vsm_buffer = data.vsm_buffer.get_bindless();
                constant.vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless();
                constant.vsm_physical_shadow_map = data.vsm_physical_shadow_map.get_bindless();
            }

            command.set_pipeline({
                .compute_shader = device.get_shader<aerial_perspective_lut_cs>(defines),
            });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);

            command.set_constant(constant);

            auto extent = data.aerial_perspective_lut.get_extent();
            command.dispatch_3d(extent.width, extent.height, extent.depth, 8, 8, 1);
        });
}

void atmosphere_lut_pass::add_environment_map_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv sky_view_lut;
        rdg_texture_uav environment_map;
    };

    graph.add_pass<pass_data>(
        "Environment Map Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.sky_view_lut = pass.add_texture_srv(
                parameter.sky_view_lut,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_TEXTURE_DIMENSION_2D);
            data.environment_map = pass.add_texture_uav(
                m_environment_map,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_TEXTURE_DIMENSION_2D_ARRAY);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<environment_map_cs>(),
            });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            rhi_extent extent = data.environment_map.get_extent();
            command.set_constant(
                environment_map_cs::constant_data{
                    .sky_view_lut = data.sky_view_lut.get_bindless(),
                    .environment_map = data.environment_map.get_bindless(),
                    .width = extent.width,
                    .height = extent.height,
                });

            command.dispatch_3d(extent.width, extent.height, 6, 8, 8, 1);
        });
}

void atmosphere_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Atmosphere");

    add_sky_pass(graph, parameter);
    add_aerial_perspective_pass(graph, parameter);
}

void atmosphere_pass::add_sky_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv sky_view_lut;

        atmosphere_data atmosphere;
        vec3f sun_direction;
        vec3f sun_irradiance;

        rhi_texture_srv* transmittance_lut;
        vec2f transmittance_lut_uv;
    };

    graph.add_pass<pass_data>(
        "Sky Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            rhi_attachment_load_op load_op =
                parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD;

            pass.add_render_target(parameter.render_target, load_op);
            pass.set_depth_stencil(parameter.depth_buffer, load_op);

            data.sky_view_lut =
                pass.add_texture_srv(parameter.sky_view_lut, RHI_PIPELINE_STAGE_FRAGMENT);

            const auto& context = graph.get_context();
            data.atmosphere = context.get_atmosphere();
            data.sun_direction = context.get_sun_direction();
            data.sun_irradiance = context.get_sun_irradiance();

            data.transmittance_lut = context.get_transmittance_lut()->get_srv();
            data.transmittance_lut_uv = get_transmittance_lut_uv(
                context.get_camera_position(),
                data.sun_direction,
                data.atmosphere.planet_radius,
                data.atmosphere.atmosphere_radius);
            ;
        },
        [](const pass_data& data, rdg_command& command)
        {
            command.set_viewport();
            command.set_scissor();

            auto& device = render_device::instance();

            rhi_sampler* sky_view_lut_sampler = device.get_sampler({
                .mag_filter = RHI_FILTER_LINEAR,
                .min_filter = RHI_FILTER_LINEAR,
                .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
                .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT,
                .min_level = 0.0f,
                .max_level = -1.0f,
            });

            std::vector<std::wstring> defines = {L"-DDYNAMIC_SKY"};

            rdg_raster_pipeline pipeline = {
                .vertex_shader = device.get_shader<atmosphere_vs>(defines),
                .fragment_shader = device.get_shader<atmosphere_fs>(defines),
                .depth_stencil_state =
                    device.get_depth_stencil_state<true, false, RHI_COMPARE_OP_EQUAL>(),
            };

            command.set_pipeline(pipeline);

            command.set_constant(
                atmosphere_constant_data{
                    .sun_direction = data.sun_direction,
                    .sun_angular_radius = data.atmosphere.sun_angular_radius,
                    .sun_irradiance = data.sun_irradiance,
                    .planet_radius = data.atmosphere.planet_radius,
                    .atmosphere_radius = data.atmosphere.atmosphere_radius,
                    .sky_view_lut = data.sky_view_lut.get_bindless(),
                    .transmittance_lut = data.transmittance_lut->get_bindless(),
                    .transmittance_lut_uv = data.transmittance_lut_uv,
                    .sky_view_lut_sampler = sky_view_lut_sampler->get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);

            command.draw(0, 36);
        });
}

void atmosphere_pass::add_aerial_perspective_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_uav render_target;
        rdg_texture_srv depth_buffer;

        rdg_texture_srv aerial_perspective_lut;
    };

    graph.add_pass<pass_data>(
        "Aerial Perspective Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.render_target =
                pass.add_texture_uav(parameter.render_target, RHI_PIPELINE_STAGE_COMPUTE);
            data.depth_buffer =
                pass.add_texture_srv(parameter.depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.aerial_perspective_lut = pass.add_texture_srv(
                parameter.aerial_perspective_lut,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_TEXTURE_DIMENSION_3D);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<aerial_perspective_cs>(),
            });

            command.set_constant(
                aerial_perspective_cs::constant_data{
                    .render_target = data.render_target.get_bindless(),
                    .depth_buffer = data.depth_buffer.get_bindless(),
                    .aerial_perspective_lut = data.aerial_perspective_lut.get_bindless(),
                    .distance_per_slice = AERIAL_PERSPECTIVE_SLICE_DISTANCE,
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_CAMERA);

            auto extent = data.render_target.get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet