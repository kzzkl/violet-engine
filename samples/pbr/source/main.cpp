#include "common/log.hpp"
#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/mesh.hpp"
#include "components/orbit_control.hpp"
#include "components/transform.hpp"
#include "control/control_module.hpp"
#include "core/engine.hpp"
#include "fbx_loader.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/geometries/sphere_geometry.hpp"
#include "graphics/geometry.hpp"
#include "graphics/graphics_module.hpp"
#include "graphics/materials/basic_material.hpp"
#include "graphics/materials/physical_material.hpp"
#include "graphics/renderers/default_renderer.hpp"
#include "graphics/tools/ibl_tool.hpp"
#include "graphics/tools/texture_loader.hpp"
#include "scene/scene_module.hpp"
#include "window/window_module.hpp"

namespace violet::sample
{
class pbr_sample : public engine_module
{
public:
    pbr_sample() : engine_module("pbr sample") {}

    bool initialize(const dictionary& config) override
    {
        m_skybox_path = config["skybox"];
        m_model_path = config["model"];
        m_albedo_path = config["albedo_tex"];
        m_normal_path = config["normal_tex"];
        m_metallic_path = config["metallic_tex"];
        m_roughness_path = config["roughness_tex"];

        on_tick().then(
            [this](float delta)
            {
                tick(delta);
            });

        auto& window = get_module<window_module>();
        window.on_resize().then(
            [this](std::uint32_t width, std::uint32_t height)
            {
                resize(width, height);
            });

        initialize_render();
        initialize_scene();

        auto extent = window.get_extent();
        resize(extent.width, extent.height);

        return true;
    }

private:
    void initialize_render()
    {
        auto window_extent = get_module<window_module>().get_extent();

        rhi_swapchain_desc swapchain_desc = {};
        swapchain_desc.width = window_extent.width;
        swapchain_desc.height = window_extent.height;
        swapchain_desc.window_handle = get_module<window_module>().get_handle();
        m_swapchain = render_device::instance().create_swapchain(swapchain_desc);
        m_renderer = std::make_unique<default_renderer>();

        rhi_ptr<rhi_texture> env_map =
            texture_loader::load(m_skybox_path, TEXTURE_LOAD_OPTION_GENERATE_MIPMAPS);

        rhi_texture_desc skybox_desc = {};
        skybox_desc.extent.width = 2048;
        skybox_desc.extent.height = 2048;
        skybox_desc.format = RHI_FORMAT_R8G8B8A8_UNORM;
        skybox_desc.layer_count = 6;
        skybox_desc.flags =
            RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE | RHI_TEXTURE_RENDER_TARGET;
        m_skybox = render_device::instance().create_texture(skybox_desc);

        rhi_texture_desc irradiance_desc = {};
        irradiance_desc.extent.width = 32;
        irradiance_desc.extent.height = 32;
        irradiance_desc.format = RHI_FORMAT_R8G8B8A8_UNORM;
        irradiance_desc.layer_count = 6;
        irradiance_desc.flags =
            RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE | RHI_TEXTURE_RENDER_TARGET;
        m_irradiance_map = render_device::instance().create_texture(irradiance_desc);

        rhi_texture_desc prefilter_desc = {};
        prefilter_desc.extent.width = 512;
        prefilter_desc.extent.height = 512;
        prefilter_desc.format = RHI_FORMAT_R8G8B8A8_UNORM;
        prefilter_desc.level_count = 10;
        prefilter_desc.layer_count = 6;
        prefilter_desc.flags =
            RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE | RHI_TEXTURE_RENDER_TARGET;
        m_prefilter_map = render_device::instance().create_texture(prefilter_desc);

        rhi_texture_desc brdf_desc = {};
        brdf_desc.extent.width = 512;
        brdf_desc.extent.height = 512;
        brdf_desc.format = RHI_FORMAT_R32G32_FLOAT;
        brdf_desc.flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_RENDER_TARGET;
        m_brdf_lut = render_device::instance().create_texture(brdf_desc);

        ibl_tool::generate(
            env_map.get(),
            m_skybox.get(),
            m_irradiance_map.get(),
            m_prefilter_map.get(),
            m_brdf_lut.get());

        rhi_sampler_desc sampler_desc = {};
        sampler_desc.min_filter = RHI_FILTER_LINEAR;
        sampler_desc.mag_filter = RHI_FILTER_LINEAR;
        m_sampler = render_device::instance().create_sampler(sampler_desc);

        rhi_sampler_desc prefilter_sampler_desc = {};
        prefilter_sampler_desc.min_filter = RHI_FILTER_LINEAR;
        prefilter_sampler_desc.mag_filter = RHI_FILTER_LINEAR;
        prefilter_sampler_desc.min_level = 0.0f;
        prefilter_sampler_desc.max_level = 4.0f;
        m_prefilter_sampler = render_device::instance().create_sampler(prefilter_sampler_desc);
    }

    void initialize_scene()
    {
        m_light = std::make_unique<actor>("light", get_world());
        auto [light_transform, main_light] = m_light->add<transform, light>();
        light_transform->set_position(float3{10.0f, 10.0f, 10.0f});
        light_transform->lookat(float3{0.0f, 0.0f, 0.0f}, float3{0.0f, 1.0f, 0.0f});

        main_light->type = LIGHT_DIRECTIONAL;
        main_light->color = float3{1.0f, 1.0f, 1.0f};

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

        m_cube = std::make_unique<actor>("cube", get_world());
        auto [cube_mesh, cube_transform] = m_cube->add<mesh, transform>();
        cube_mesh->set_geometry(m_geometry.get());
        cube_mesh->add_submesh(
            0,
            m_geometry->get_vertex_count(),
            0,
            m_geometry->get_index_count(),
            m_material.get());
        cube_transform->set_position({-model_center.x, -model_center.y, -model_center.z});

        m_camera = std::make_unique<actor>("main camera", get_world());
        auto [camera_transform, main_camera, camera_control] =
            m_camera->add<transform, camera, orbit_control>();
        camera_transform->set_position(float3{0.0f, 0.0f, -10.0f});

        main_camera->set_skybox(m_skybox.get(), m_sampler.get());
        main_camera->set_renderer(m_renderer.get());
    }

    void tick(float delta) {}

    void resize(std::uint32_t width, std::uint32_t height)
    {
        render_device& device = render_device::instance();

        m_swapchain->resize(width, height);

        rhi_texture_desc depth_buffer_desc = {};
        depth_buffer_desc.extent.width = width;
        depth_buffer_desc.extent.height = height;
        depth_buffer_desc.format = RHI_FORMAT_D24_UNORM_S8_UINT;
        depth_buffer_desc.samples = RHI_SAMPLE_COUNT_1;
        depth_buffer_desc.flags = RHI_TEXTURE_DEPTH_STENCIL;
        m_depth_buffer = device.create_texture(depth_buffer_desc);

        rhi_texture_desc light_buffer_desc = {};
        light_buffer_desc.extent.width = width;
        light_buffer_desc.extent.height = height;
        light_buffer_desc.format = RHI_FORMAT_R8G8B8A8_UNORM;
        light_buffer_desc.samples = RHI_SAMPLE_COUNT_1;
        light_buffer_desc.flags = RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE;

        auto main_camera = m_camera->get<camera>();
        main_camera->resize(width, height);
        main_camera->set_render_target(0, m_swapchain.get());
        main_camera->set_render_target(1, m_depth_buffer.get());
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
    std::unique_ptr<actor> m_cube;

    std::unique_ptr<actor> m_light;
    std::unique_ptr<actor> m_camera;

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
    using namespace violet;

    engine engine;
    engine.initialize("pbr/config");
    engine.install<scene_module>();
    engine.install<window_module>();
    engine.install<graphics_module>();
    engine.install<control_module>();
    engine.install<sample::pbr_sample>();

    engine.get_module<window_module>().on_destroy().then(
        [&engine]()
        {
            engine.exit();
        });

    engine.run();

    return 0;
}