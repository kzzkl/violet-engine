#include "common/log.hpp"
#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/mesh.hpp"
#include "components/orbit_control.hpp"
#include "components/skybox.hpp"
#include "components/transform.hpp"
#include "control/control_system.hpp"
#include "core/engine.hpp"
#include "ecs_command/ecs_command_system.hpp"
#include "fbx_loader.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/geometries/sphere_geometry.hpp"
#include "graphics/geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/basic_material.hpp"
#include "graphics/materials/physical_material.hpp"
#include "graphics/renderers/default_renderer.hpp"
#include "graphics/tools/ibl_tool.hpp"
#include "graphics/tools/texture_loader.hpp"
#include "scene/hierarchy_system.hpp"
#include "scene/scene_system.hpp"
#include "scene/transform_system.hpp"
#include "task/task_graph_printer.hpp"
#include "window/window_system.hpp"

namespace violet::sample
{
class pbr_sample : public engine_system
{
public:
    pbr_sample()
        : engine_system("PBR Sample")
    {
    }

    bool initialize(const dictionary& config) override
    {
        m_skybox_path = config["skybox"];
        m_model_path = config["model"];
        m_albedo_path = config["albedo_tex"];
        m_normal_path = config["normal_tex"];
        m_metallic_path = config["metallic_tex"];
        m_roughness_path = config["roughness_tex"];

        auto& window = get_system<window_system>();
        window.on_resize().add_task().set_execute(
            [this]()
            {
                resize();
            });
        window.on_destroy().add_task().set_execute(
            []()
            {
                engine::exit();
            });

        task_graph& task_graph = get_task_graph();
        task_group& update = task_graph.get_group("Update Group");

        task_graph.add_task()
            .set_name("PBR Tick")
            .set_group(update)
            .set_execute(
                [this]()
                {
                    tick();
                });

        task_graph.reset();
        task_graph_printer::print(task_graph);

        initialize_render();
        initialize_scene();

        resize();

        return true;
    }

private:
    void initialize_render()
    {
        auto& device = render_device::instance();

        auto window_extent = get_system<window_system>().get_extent();

        rhi_swapchain_desc swapchain_desc = {};
        swapchain_desc.width = window_extent.width;
        swapchain_desc.height = window_extent.height;
        swapchain_desc.window_handle = get_system<window_system>().get_handle();
        m_swapchain = device.create_swapchain(swapchain_desc);
        m_renderer = std::make_unique<default_renderer>();

        rhi_ptr<rhi_texture> env_map =
            texture_loader::load(m_skybox_path, TEXTURE_LOAD_OPTION_GENERATE_MIPMAPS);

        m_skybox = device.create_texture(
            {.extent = {2048, 2048},
             .format = RHI_FORMAT_R8G8B8A8_UNORM,
             .level_count = 1,
             .layer_count = 6,
             .samples = RHI_SAMPLE_COUNT_1,
             .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE | RHI_TEXTURE_RENDER_TARGET});
        device.set_name(m_skybox.get(), "Skybox");

        m_irradiance_map = device.create_texture(
            {.extent = {32, 32},
             .format = RHI_FORMAT_R8G8B8A8_UNORM,
             .level_count = 1,
             .layer_count = 6,
             .samples = RHI_SAMPLE_COUNT_1,
             .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE | RHI_TEXTURE_RENDER_TARGET});
        device.set_name(m_irradiance_map.get(), "Irradiance");

        m_prefilter_map = device.create_texture(
            {.extent = {512, 512},
             .format = RHI_FORMAT_R8G8B8A8_UNORM,
             .level_count = 10,
             .layer_count = 6,
             .samples = RHI_SAMPLE_COUNT_1,
             .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE | RHI_TEXTURE_RENDER_TARGET});
        device.set_name(m_prefilter_map.get(), "Prefilter Map");

        m_brdf_lut = device.create_texture(
            {.extent = {512, 512},
             .format = RHI_FORMAT_R32G32_FLOAT,
             .level_count = 1,
             .layer_count = 1,
             .samples = RHI_SAMPLE_COUNT_1,
             .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_RENDER_TARGET});
        device.set_name(m_brdf_lut.get(), "BRDF LUT");

        ibl_tool::generate(
            env_map.get(),
            m_skybox.get(),
            m_irradiance_map.get(),
            m_prefilter_map.get(),
            m_brdf_lut.get());

        rhi_sampler_desc sampler_desc = {};
        sampler_desc.min_filter = RHI_FILTER_LINEAR;
        sampler_desc.mag_filter = RHI_FILTER_LINEAR;
        m_sampler = device.create_sampler(sampler_desc);

        rhi_sampler_desc prefilter_sampler_desc = {};
        prefilter_sampler_desc.min_filter = RHI_FILTER_LINEAR;
        prefilter_sampler_desc.mag_filter = RHI_FILTER_LINEAR;
        prefilter_sampler_desc.min_level = 0.0f;
        prefilter_sampler_desc.max_level = 4.0f;
        m_prefilter_sampler = device.create_sampler(prefilter_sampler_desc);
    }

    void initialize_scene()
    {
        auto& world = get_world();

        m_light = world.create();
        world.add_component<transform, light>(m_light);

        auto& light_transform = world.get_component<transform>(m_light);
        light_transform.position = {10.0f, 10.0f, 10.0f};
        light_transform.lookat(float3{0.0f, 0.0f, 0.0f}, float3{0.0f, 1.0f, 0.0f});

        auto& main_light = world.get_component<light>(m_light);
        main_light.type = LIGHT_DIRECTIONAL;
        main_light.color = {1.0f, 1.0f, 1.0f};

        float3 model_center = {};

        // m_geometry = std::make_unique<box_geometry>();
        m_geometry = fbx_loader::load(m_model_path, &model_center);

        m_albedo = texture_loader::load(m_albedo_path, TEXTURE_LOAD_OPTION_SRGB);
        m_normal = texture_loader::load(m_normal_path);
        m_metallic = texture_loader::load(m_metallic_path);
        m_roughness = texture_loader::load(m_roughness_path);

        m_material = std::make_unique<physical_material>(true);
        m_material->set_normal(m_normal.get(), m_sampler.get());
        m_material->set_albedo(m_albedo.get(), m_sampler.get());
        m_material->set_metallic(m_metallic.get(), m_sampler.get());
        m_material->set_roughness(m_roughness.get(), m_sampler.get());
        // m_material->set_albedo(float3{1.0f, 1.0f, 1.0f});
        // m_material->set_metallic(1.0f);
        // m_material->set_roughness(0.0f);
        m_material->set_irradiance_map(m_irradiance_map.get(), m_sampler.get());
        m_material->set_prefilter_map(m_prefilter_map.get(), m_prefilter_sampler.get());
        m_material->set_brdf_lut(m_brdf_lut.get(), m_sampler.get());

        m_model = world.create();
        world.add_component<transform, transform_local, transform_world, mesh>(m_model);

        auto& cube_mesh = world.get_component<mesh>(m_model);
        cube_mesh.geometry = m_geometry.get();
        cube_mesh.submeshes.emplace_back(0, 0, 0, m_geometry->get_index_count(), m_material.get());

        auto& cube_transform = world.get_component<transform>(m_model);
        cube_transform.position = {-model_center.x, -model_center.y, -model_center.z};

        m_camera = world.create();
        world.add_component<
            transform,
            transform_local,
            transform_world,
            camera,
            skybox,
            orbit_control>(m_camera);

        auto& camera_transform = world.get_component<transform>(m_camera);
        camera_transform.position = {0.0f, 0.0f, -10.0f};

        auto& main_camera = world.get_component<camera>(m_camera);
        main_camera.renderer = m_renderer.get();
        main_camera.render_targets.resize(2);
        main_camera.viewport.min_depth = 0.0f;
        main_camera.viewport.max_depth = 1.0f;

        auto& camera_skybox = world.get_component<skybox>(m_camera);
        camera_skybox.texture = m_skybox.get();
        camera_skybox.sampler = m_sampler.get();
    }

    void resize()
    {
        auto extent = get_system<window_system>().get_extent();

        render_device& device = render_device::instance();

        m_swapchain->resize(extent.width, extent.height);

        rhi_texture_desc depth_buffer_desc = {};
        depth_buffer_desc.extent.width = extent.width;
        depth_buffer_desc.extent.height = extent.height;
        depth_buffer_desc.format = RHI_FORMAT_D24_UNORM_S8_UINT;
        depth_buffer_desc.samples = RHI_SAMPLE_COUNT_1;
        depth_buffer_desc.flags = RHI_TEXTURE_DEPTH_STENCIL;
        m_depth_buffer = device.create_texture(depth_buffer_desc);

        rhi_texture_desc light_buffer_desc = {};
        light_buffer_desc.extent.width = extent.width;
        light_buffer_desc.extent.height = extent.height;
        light_buffer_desc.format = RHI_FORMAT_R8G8B8A8_UNORM;
        light_buffer_desc.samples = RHI_SAMPLE_COUNT_1;
        light_buffer_desc.flags = RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE;

        auto& main_camera = get_world().get_component<camera>(m_camera);
        main_camera.render_targets[0] = m_swapchain.get();
        main_camera.render_targets[1] = m_depth_buffer.get();
        main_camera.viewport.width = extent.width;
        main_camera.viewport.height = extent.height;

        float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
        math::store(matrix::perspective(45.0f, aspect, 0.1f, 1000.0f), main_camera.projection);
    }

    void tick()
    {
        /*auto& camera_transform = get_world().get_component<const transform>(m_camera);
        log::debug(
            "{} {} {} {}",
            camera_transform.rotation.x,
            camera_transform.rotation.y,
            camera_transform.rotation.z,
            camera_transform.rotation.w);*/
    }

    rhi_ptr<rhi_swapchain> m_swapchain;
    rhi_ptr<rhi_texture> m_depth_buffer;

    rhi_ptr<rhi_texture> m_irradiance_map;
    rhi_ptr<rhi_texture> m_prefilter_map;
    rhi_ptr<rhi_sampler> m_prefilter_sampler;
    rhi_ptr<rhi_texture> m_brdf_lut;
    rhi_ptr<rhi_texture> m_skybox;
    rhi_ptr<rhi_sampler> m_sampler;

    rhi_ptr<rhi_texture> m_albedo;
    rhi_ptr<rhi_texture> m_normal;
    rhi_ptr<rhi_texture> m_metallic;
    rhi_ptr<rhi_texture> m_roughness;

    std::unique_ptr<geometry> m_geometry;
    std::unique_ptr<physical_material> m_material;
    entity m_model;

    entity m_light;
    entity m_camera;

    std::unique_ptr<renderer> m_renderer;

    std::string m_skybox_path;
    std::string m_model_path;
    std::string m_albedo_path;
    std::string m_normal_path;
    std::string m_metallic_path;
    std::string m_roughness_path;
};
} // namespace violet::sample

int main()
{
    violet::engine::initialize("pbr/config");
    violet::engine::install<violet::ecs_command_system>();
    violet::engine::install<violet::hierarchy_system>();
    violet::engine::install<violet::transform_system>();
    violet::engine::install<violet::scene_system>();
    violet::engine::install<violet::window_system>();
    violet::engine::install<violet::graphics_system>();
    violet::engine::install<violet::control_system>();
    violet::engine::install<violet::sample::pbr_sample>();

    violet::engine::run();

    return 0;
}