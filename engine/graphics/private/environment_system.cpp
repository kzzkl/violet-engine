#include "environment_system.hpp"
#include "components/atmosphere_component_meta.hpp"
#include "components/light_component_meta.hpp"
#include "components/scene_component.hpp"
#include "components/skybox_component_meta.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "graphics/renderers/passes/ibl_pass.hpp"

namespace violet
{
struct hdri_convert_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/hdri_to_cubemap.hlsl";

    struct constant_data
    {
        std::uint32_t hdri_texture;
        std::uint32_t environment_map;
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
        atmosphere_data atmosphere;

        std::uint32_t transmittance_lut;
        std::uint32_t sample_count;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct multi_scattering_lut_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/multi_scattering_lut.hlsl";

    struct constant_data
    {
        atmosphere_data atmosphere;

        std::uint32_t transmittance_lut;
        std::uint32_t multi_scattering_lut;
        std::uint32_t sample_count;
        std::uint32_t direction_count;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

class environment_renderer
{
public:
    struct skybox_parameter
    {
        rhi_texture* hdri;
        rhi_texture* environment_map;
        rhi_texture* prefilter_map;
        rhi_buffer* irradiance_sh;
    };

    void render_skybox(rhi_command* command, const skybox_parameter& parameter)
    {
        render_graph graph("Update Skybox");

        m_environment_map = graph.add_texture(
            "Environment Map",
            parameter.environment_map,
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        m_prefilter_map = graph.add_texture(
            "Prefilter Map",
            parameter.prefilter_map,
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        m_irradiance_sh = graph.add_buffer("Irradiance SH", parameter.irradiance_sh);

        if (parameter.hdri != nullptr)
        {
            add_hdri_convert_pass(graph, parameter.hdri);
        }

        graph.add_pass<prefilter_pass>({
            .environment_map = m_environment_map,
            .prefilter_map = m_prefilter_map,
        });

        graph.add_pass<irradiance_pass>({
            .environment_map = m_environment_map,
            .irradiance_sh = m_irradiance_sh,
        });

        graph.compile();
        graph.record(command);
    }

    struct atmosphere_parameter
    {
        atmosphere atmosphere;
        rhi_texture* transmittance_lut;
        rhi_texture* multi_scattering_lut;
    };

    void render_atmosphere(rhi_command* command, const atmosphere_parameter& parameter)
    {
        render_graph graph("Update Atmosphere");

        m_transmittance_lut = graph.add_texture(
            "Transmittance LUT",
            parameter.transmittance_lut,
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        m_multi_scattering_lut = graph.add_texture(
            "Multi Scattering LUT",
            parameter.multi_scattering_lut,
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        add_transmittance_lut_pass(graph, parameter);
        add_multi_scattering_lut_pass(graph, parameter);

        graph.compile();
        graph.record(command);
    }

private:
    void add_hdri_convert_pass(render_graph& graph, rhi_texture* hdri)
    {
        rdg_texture* hdri_texture = graph.add_texture(
            "HDRI",
            hdri,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            RHI_TEXTURE_LAYOUT_UNDEFINED);

        struct pass_data
        {
            rdg_texture_srv hdri_texture;
            rdg_texture_uav environment_map;
        };

        graph.add_pass<pass_data>(
            "HDRI Convert Pass",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.hdri_texture = pass.add_texture_srv(hdri_texture, RHI_PIPELINE_STAGE_COMPUTE);
                data.environment_map = pass.add_texture_uav(
                    m_environment_map,
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
                        .hdri_texture = data.hdri_texture.get_bindless(),
                        .environment_map = data.environment_map.get_bindless(),
                    });
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                rhi_extent extent = data.environment_map.get_texture()->get_extent();
                command.dispatch_3d(extent.width, extent.height, 6, 8, 8, 1);
            });
    }

    void add_transmittance_lut_pass(render_graph& graph, const atmosphere_parameter& parameter)
    {
        struct pass_data
        {
            rdg_texture_uav transmittance_lut;
            atmosphere_data atmosphere;
        };

        graph.add_pass<pass_data>(
            "Transmittance LUT",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.transmittance_lut =
                    pass.add_texture_uav(m_transmittance_lut, RHI_PIPELINE_STAGE_COMPUTE);
                data.atmosphere = parameter.atmosphere;
            },
            [=](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<transmittance_lut_cs>(),
                });

                command.set_constant(
                    transmittance_lut_cs::constant_data{
                        .atmosphere = data.atmosphere,
                        .transmittance_lut = data.transmittance_lut.get_bindless(),
                        .sample_count = 1024,
                    });
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                rhi_extent extent = data.transmittance_lut.get_extent();
                command.dispatch_2d(extent.width, extent.height);
            });
    }

    void add_multi_scattering_lut_pass(render_graph& graph, const atmosphere_parameter& parameter)
    {
        struct pass_data
        {
            rdg_texture_srv transmittance_lut;
            rdg_texture_uav multi_scattering_lut;
            atmosphere_data atmosphere;
        };

        graph.add_pass<pass_data>(
            "Multi Scattering LUT",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.transmittance_lut =
                    pass.add_texture_srv(m_transmittance_lut, RHI_PIPELINE_STAGE_COMPUTE);
                data.multi_scattering_lut =
                    pass.add_texture_uav(m_multi_scattering_lut, RHI_PIPELINE_STAGE_COMPUTE);
                data.atmosphere = parameter.atmosphere;
            },
            [=](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<multi_scattering_lut_cs>(),
                });

                command.set_constant(
                    multi_scattering_lut_cs::constant_data{
                        .atmosphere = data.atmosphere,
                        .transmittance_lut = data.transmittance_lut.get_bindless(),
                        .multi_scattering_lut = data.multi_scattering_lut.get_bindless(),
                        .sample_count = 10,
                        .direction_count = 64,
                    });
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                rhi_extent extent = data.multi_scattering_lut.get_extent();
                command.dispatch_2d(extent.width, extent.height);
            });
    }

    rdg_texture* m_environment_map;
    rdg_buffer* m_irradiance_sh;
    rdg_texture* m_prefilter_map;

    rdg_texture* m_transmittance_lut;
    rdg_texture* m_multi_scattering_lut;
};

environment_system::environment_system()
    : system("environment")
{
}

bool environment_system::initialize(const dictionary& config)
{
    auto& world = get_world();
    world.register_component<skybox_component>();
    world.register_component<skybox_component_meta>();
    world.register_component<atmosphere_component>();
    world.register_component<atmosphere_component_meta>();

    return true;
}

void environment_system::update(render_scene_manager& scene_manager)
{
    auto& world = get_world();
    auto& device = render_device::instance();

    world.get_view()
        .read<entity>()
        .read<skybox_component>()
        .read<scene_component>()
        .write<skybox_component_meta>()
        .each(
            [&](const entity& entity,
                const skybox_component& skybox,
                const scene_component& scene,
                skybox_component_meta& meta)
            {
                if (skybox.texture.get() != meta.texture)
                {
                    meta.texture = skybox.texture.get();
                    m_skybox_update_queue.push_back(entity);
                }

                if (meta.texture->get_flags() & RHI_TEXTURE_CUBE)
                {
                    meta.environment_map = nullptr;
                }
                else if (meta.environment_map == nullptr)
                {
                    meta.environment_map = device.create_texture({
                        .extent = {.width = 1024, .height = 1024},
                        .format = RHI_FORMAT_R11G11B10_FLOAT,
                        .flags =
                            RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE,
                        .level_count = 1,
                        .layer_count = 6,
                    });
                    meta.environment_map->set_name("Environment Map");
                }

                if (meta.irradiance_sh == nullptr)
                {
                    meta.irradiance_sh = device.create_buffer({
                        .size = 9 * sizeof(vec4f),
                        .flags = RHI_BUFFER_STORAGE,
                    });
                    meta.irradiance_sh->set_name("Irradiance SH");
                }

                if (meta.prefilter_map == nullptr)
                {
                    meta.prefilter_map = device.create_texture({
                        .extent = {.width = 256, .height = 256},
                        .format = RHI_FORMAT_R11G11B10_FLOAT,
                        .flags =
                            RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE,
                        .level_count = rhi_get_level_count({.width = 256, .height = 256}),
                        .layer_count = 6,
                    });
                    meta.prefilter_map->set_name("Prefilter Map");
                }

                render_scene* render_scene = scene_manager.get_scene(scene.layer);
                render_scene->set_skybox(
                    meta.environment_map.get(),
                    meta.irradiance_sh.get(),
                    meta.prefilter_map.get());
            },
            [this](auto& view)
            {
                return view.template is_updated<skybox_component>(m_system_version);
            });

    world.get_view()
        .read<entity>()
        .read<atmosphere_component>()
        .read<light_component_meta>()
        .read<scene_component>()
        .write<atmosphere_component_meta>()
        .each(
            [&](const entity& entity,
                const atmosphere_component& atmosphere,
                const light_component_meta& light_meta,
                const scene_component& scene,
                atmosphere_component_meta& meta)
            {
                if (meta.update(atmosphere))
                {
                    m_atmosphere_update_queue.push_back(entity);
                }

                if (meta.transmittance_lut == nullptr)
                {
                    meta.transmittance_lut = device.create_texture({
                        .extent = {.width = 256, .height = 64},
                        .format = RHI_FORMAT_R11G11B10_FLOAT,
                        .flags = RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE,
                        .level_count = 1,
                        .layer_count = 1,
                    });
                    meta.transmittance_lut->set_name("Transmittance LUT");
                }

                if (meta.multi_scattering_lut == nullptr)
                {
                    meta.multi_scattering_lut = device.create_texture({
                        .extent = {.width = 32, .height = 32},
                        .format = RHI_FORMAT_R11G11B10_FLOAT,
                        .flags = RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE,
                        .level_count = 1,
                        .layer_count = 1,
                    });
                    meta.multi_scattering_lut->set_name("Multi Scattering LUT");
                }

                render_scene* render_scene = scene_manager.get_scene(scene.layer);
                render_scene->set_atmosphere(
                    meta.atmosphere,
                    light_meta.id,
                    meta.transmittance_lut.get(),
                    meta.multi_scattering_lut.get());
            },
            [this](auto& view)
            {
                return view.template is_updated<atmosphere_component>(m_system_version);
            });

    m_system_version = world.get_version();
}

void environment_system::record(rhi_command* command)
{
    for (auto entity : m_skybox_update_queue)
    {
        update_skybox(command, entity);
    }

    m_skybox_update_queue.clear();

    for (auto entity : m_atmosphere_update_queue)
    {
        update_atmosphere(command, entity);
    }

    m_atmosphere_update_queue.clear();
}

void environment_system::update_skybox(rhi_command* command, entity entity)
{
    auto& world = get_world();
    const auto& meta = world.get_component<const skybox_component_meta>(entity);

    environment_renderer renderer;

    environment_renderer::skybox_parameter parameter = {
        .prefilter_map = meta.prefilter_map.get(),
        .irradiance_sh = meta.irradiance_sh.get(),
    };

    if (meta.texture->get_flags() & RHI_TEXTURE_CUBE)
    {
        parameter.environment_map = meta.texture;
    }
    else
    {
        parameter.hdri = meta.texture;
        parameter.environment_map = meta.environment_map.get();
    }

    renderer.render_skybox(command, parameter);
}

void environment_system::update_atmosphere(rhi_command* command, entity entity)
{
    auto& world = get_world();
    const auto& meta = world.get_component<const atmosphere_component_meta>(entity);

    environment_renderer renderer;
    renderer.render_atmosphere(
        command,
        environment_renderer::atmosphere_parameter{
            .atmosphere = meta.atmosphere,
            .transmittance_lut = meta.transmittance_lut.get(),
            .multi_scattering_lut = meta.multi_scattering_lut.get(),
        });
}
} // namespace violet