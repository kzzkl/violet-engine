#include "graphics/skybox.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
struct hdri_convert_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/hdri_to_cubemap.hlsl";

    struct constant_data
    {
        std::uint32_t env_map;
        std::uint32_t cube_map;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct irradiance_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/irradiance.hlsl";

    struct constant_data
    {
        std::uint32_t cube_map;
        std::uint32_t irradiance_map;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct prefilter_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/prefilter.hlsl";

    struct constant_data
    {
        std::uint32_t cube_map;
        std::uint32_t prefilter_map;
        float roughness;
        std::uint32_t resolution;
        std::uint32_t width;
        std::uint32_t height;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct transmittance_lut_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/transmittance_lut.hlsl";

    struct constant_data
    {
        skybox::atmosphere_data atmosphere;

        std::uint32_t transmittance_lut;
        std::uint32_t sample_count;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

skybox::skybox(
    const rhi_extent& irradiance_extent,
    const rhi_extent& prefilter_extent,
    const rhi_extent& skybox_texture_extent,
    const rhi_extent& transmittance_lut_extent)
    : m_irradiance_extent(irradiance_extent),
      m_prefilter_extent(prefilter_extent),
      m_skybox_texture_extent(skybox_texture_extent),
      m_transmittance_lut_extent(transmittance_lut_extent)
{
}

void skybox::set_dynamic_sky(bool dynamic_sky)
{
    m_dynamic_sky = dynamic_sky;
    m_dirty = true;

    if (m_dynamic_sky)
    {
        if (m_transmittance_lut == nullptr)
        {
            m_transmittance_lut = std::make_unique<texture_2d>(
                m_transmittance_lut_extent,
                RHI_FORMAT_R11G11B10_FLOAT,
                RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE);
            m_transmittance_lut->get_rhi()->set_name("Transmittance LUT");
        }
    }
    else
    {
    }
}

void skybox::set_skybox_texture(std::string_view path)
{
    if (m_skybox_texture_path != path)
    {
        m_skybox_texture_path = path;
        m_dirty = true;
    }

    if (m_skybox_texture == nullptr)
    {
        m_skybox_texture = std::make_unique<texture_cube>(
            m_skybox_texture_extent,
            RHI_FORMAT_R11G11B10_FLOAT,
            RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_SRC |
                RHI_TEXTURE_TRANSFER_DST | RHI_TEXTURE_CUBE);
        m_skybox_texture->get_rhi()->set_name("Skybox");
    }
}

void skybox::update(rhi_command* command)
{
    if (!m_dirty)
    {
        return;
    }

    m_dirty = false;

    if (m_irradiance == nullptr || m_irradiance->get_rhi()->get_extent() != m_irradiance_extent)
    {
        m_irradiance = std::make_unique<texture_cube>(
            m_irradiance_extent,
            RHI_FORMAT_R11G11B10_FLOAT,
            RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE);
        m_irradiance->get_rhi()->set_name("Skybox Irradiance");
    }

    if (m_prefilter == nullptr || m_prefilter->get_rhi()->get_extent() != m_prefilter_extent)
    {
        m_prefilter = std::make_unique<texture_cube>(
            m_prefilter_extent,
            RHI_FORMAT_R11G11B10_FLOAT,
            RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE);
        m_prefilter->get_rhi()->set_name("Skybox Prefilter");
    }

    if (m_dynamic_sky)
    {
        update_atmosphere(command);
    }
    else
    {
        update_texture(command);
    }
}

void skybox::update_texture(rhi_command* command)
{
    texture_2d env_map(m_skybox_texture_path);

    render_graph graph("Update Static Sky");

    rdg_texture* skybox_texture = graph.add_texture(
        "Cube Map",
        m_skybox_texture->get_rhi(),
        RHI_TEXTURE_LAYOUT_UNDEFINED,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

    // Convert HDRI to Cube Map.
    {
        rdg_texture* environment_map = graph.add_texture(
            "Environment Map",
            env_map.get_rhi(),
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        struct pass_data
        {
            rdg_texture_srv env_map;
            rdg_texture_uav cube_map;
        };

        graph.add_pass<pass_data>(
            "HDRI Convert Pass",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.env_map = pass.add_texture_srv(environment_map, RHI_PIPELINE_STAGE_COMPUTE);
                data.cube_map = pass.add_texture_uav(
                    skybox_texture,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D_ARRAY);
            },
            [](const pass_data& data, rdg_command& command)
            {
                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<hdri_convert_cs>(),
                });
                command.set_constant(
                    hdri_convert_cs::constant_data{
                        .env_map = data.env_map.get_bindless(),
                        .cube_map = data.cube_map.get_bindless(),
                    });
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                rhi_extent extent = data.cube_map.get_texture()->get_extent();
                command.dispatch_3d(extent.width, extent.height, 6, 8, 8, 1);
            });
    }

    // Generate Irradiance Map.
    {
        rdg_texture* irradiance_map = graph.add_texture(
            "Irradiance Map",
            m_irradiance->get_rhi(),
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        struct pass_data
        {
            rdg_texture_srv cube_map;
            rdg_texture_uav irradiance_map;
        };

        graph.add_pass<pass_data>(
            "Irradiance Pass",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.cube_map = pass.add_texture_srv(
                    skybox_texture,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_CUBE);
                data.irradiance_map = pass.add_texture_uav(
                    irradiance_map,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D_ARRAY);
            },
            [](const pass_data& data, rdg_command& command)
            {
                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<irradiance_cs>(),
                });
                command.set_constant(
                    irradiance_cs::constant_data{
                        .cube_map = data.cube_map.get_bindless(),
                        .irradiance_map = data.irradiance_map.get_bindless(),
                    });
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                rhi_extent extent = data.irradiance_map.get_texture()->get_extent();
                command.dispatch_3d(extent.width, extent.height, 6, 8, 8, 1);
            });
    }

    // Generate Prefilter Map.
    {
        rdg_texture* prefilter_map = graph.add_texture(
            "Prefilter Map",
            m_prefilter->get_rhi(),
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        struct pass_data
        {
            rdg_texture_srv cube_map;
            rdg_texture_uav prefilter_map;
        };

        for (std::uint32_t level = 0; level < prefilter_map->get_level_count(); ++level)
        {
            graph.add_pass<pass_data>(
                "Prefilter Pass Mip " + std::to_string(level),
                RDG_PASS_COMPUTE,
                [&](pass_data& data, rdg_pass& pass)
                {
                    data.cube_map = pass.add_texture_srv(
                        skybox_texture,
                        RHI_PIPELINE_STAGE_COMPUTE,
                        RHI_TEXTURE_DIMENSION_CUBE);
                    data.prefilter_map = pass.add_texture_uav(
                        prefilter_map,
                        RHI_PIPELINE_STAGE_COMPUTE,
                        RHI_TEXTURE_DIMENSION_2D_ARRAY,
                        level,
                        1);
                },
                [](const pass_data& data, rdg_command& command)
                {
                    std::uint32_t level = data.prefilter_map.get_level();
                    std::uint32_t level_count = data.prefilter_map.get_texture()->get_level_count();

                    command.set_pipeline({
                        .compute_shader = render_device::instance().get_shader<prefilter_cs>(),
                    });

                    rhi_extent extent = data.prefilter_map.get_extent();
                    command.set_constant(
                        prefilter_cs::constant_data{
                            .cube_map = data.cube_map.get_bindless(),
                            .prefilter_map = data.prefilter_map.get_bindless(),
                            .roughness =
                                static_cast<float>(level) / static_cast<float>(level_count),
                            .resolution = data.cube_map.get_texture()->get_extent().width,
                            .width = extent.width,
                            .height = extent.height,
                        });

                    command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                    command.dispatch_3d(extent.width, extent.height, 6, 8, 8, 1);
                });
        }
    }

    graph.compile();
    graph.record(command);
}

void skybox::update_atmosphere(rhi_command* command)
{
    render_graph graph("Update Dynamic Sky");

    rdg_texture* output = graph.add_texture(
        "Transmittance LUT",
        m_transmittance_lut->get_rhi(),
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
        [=, this](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<transmittance_lut_cs>(),
            });

            command.set_constant(
                transmittance_lut_cs::constant_data{
                    .atmosphere = m_atmosphere,
                    .transmittance_lut = data.transmittance_lut.get_bindless(),
                    .sample_count = 1024,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            rhi_extent extent = data.transmittance_lut.get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });

    graph.compile();
    graph.record(command);
}
} // namespace violet