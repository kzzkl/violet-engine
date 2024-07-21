#include "common/log.hpp"
#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/mesh.hpp"
#include "components/orbit_control.hpp"
#include "components/transform.hpp"
#include "control/control_module.hpp"
#include "core/engine.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/geometries/sphere_geometry.hpp"
#include "graphics/geometry.hpp"
#include "graphics/graphics_module.hpp"
#include "graphics/materials/basic_material.hpp"
#include "graphics/renderers/default_renderer.hpp"
#include "graphics/tools/ibl_tool.hpp"
#include "scene/scene_module.hpp"
#include "window/window_module.hpp"

namespace violet::sample
{
class test_material : public material
{
public:
    struct test_vs : public vertex_shader<test_vs>
    {
        static constexpr std::string_view path = "pbr/shaders/test.vs";

        static constexpr input inputs[] = {
            {"position", RHI_FORMAT_R32G32B32_FLOAT}
        };
        static constexpr parameter_slot parameters[] = {
            {0, shader::camera},
            {1, shader::light },
            {2, shader::mesh  }
        };
    };

    struct test_fs : public fragment_shader<test_fs>
    {
        static constexpr parameter material = {
            {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1}
        };

        static constexpr std::string_view path = "pbr/shaders/test.fs";
        static constexpr parameter_slot parameters[] = {
            {3, material}
        };
    };

public:
    test_material()
    {
        rdg_render_pipeline pipeline = {};
        pipeline.vertex_shader = test_vs::get_rhi();
        pipeline.fragment_shader = test_fs::get_rhi();
        add_pass(pipeline, test_fs::material);
    }

    void set_texture(rhi_texture* texture, rhi_sampler* sampler)
    {
        get_parameter(0)->set_texture(0, texture, sampler);
    }
};

class pbr_sample : public engine_module
{
public:
    pbr_sample() : engine_module("pbr sample") {}

    bool initialize(const dictionary& config) override
    {
        m_skybox_path = config["skybox"];

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

        rhi_texture_desc env_desc = {};
        env_desc.create_type = RHI_TEXTURE_CREATE_FROM_FILE;
        env_desc.file.paths[0] = m_skybox_path.c_str();
        env_desc.flags = RHI_TEXTURE_SHADER_RESOURCE;
        m_env_map = render_device::instance().create_texture(env_desc);

        rhi_texture_desc skybox_desc = {};
        skybox_desc.create_type = RHI_TEXTURE_CREATE_FROM_INFO;
        skybox_desc.info.extent.width = 2048;
        skybox_desc.info.extent.height = 2048;
        skybox_desc.info.format = RHI_FORMAT_R8G8B8A8_UNORM;
        skybox_desc.flags =
            RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE | RHI_TEXTURE_RENDER_TARGET;
        m_skybox = render_device::instance().create_texture(skybox_desc);

        rhi_texture_desc irradiance_desc = {};
        irradiance_desc.create_type = RHI_TEXTURE_CREATE_FROM_INFO;
        irradiance_desc.info.extent.width = 32;
        irradiance_desc.info.extent.height = 32;
        irradiance_desc.info.format = RHI_FORMAT_R8G8B8A8_UNORM;
        irradiance_desc.flags =
            RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE | RHI_TEXTURE_RENDER_TARGET;
        m_irradiance_map = render_device::instance().create_texture(irradiance_desc);

        rhi_texture_desc prefilter_desc = {};
        prefilter_desc.create_type = RHI_TEXTURE_CREATE_FROM_INFO;
        prefilter_desc.info.extent.width = 512;
        prefilter_desc.info.extent.height = 512;
        prefilter_desc.info.format = RHI_FORMAT_R8G8B8A8_UNORM;
        prefilter_desc.flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE |
                               RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_MIPMAP;
        m_prefilter_map = render_device::instance().create_texture(prefilter_desc);

        rhi_texture_desc brdf_desc = {};
        brdf_desc.create_type = RHI_TEXTURE_CREATE_FROM_INFO;
        brdf_desc.info.extent.width = 512;
        brdf_desc.info.extent.height = 512;
        brdf_desc.info.format = RHI_FORMAT_R32G32_FLOAT;
        brdf_desc.flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_RENDER_TARGET;
        m_brdf_lut = render_device::instance().create_texture(brdf_desc);

        ibl_tool::generate(
            m_env_map.get(),
            m_skybox.get(),
            m_irradiance_map.get(),
            m_prefilter_map.get(),
            m_brdf_lut.get());

        rhi_sampler_desc sampler_desc = {};
        sampler_desc.min_filter = RHI_FILTER_LINEAR;
        sampler_desc.mag_filter = RHI_FILTER_LINEAR;
        sampler_desc.address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_desc.address_mode_v = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_desc.address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        m_skybox_sampler = render_device::instance().create_sampler(sampler_desc);
    }

    void initialize_scene()
    {
        m_geometry = std::make_unique<box_geometry>();
        m_material = std::make_unique<test_material>();
        m_material->set_texture(m_env_map.get(), m_skybox_sampler.get());

        m_cube = std::make_unique<actor>("cube", get_world());
        auto [cube_mesh, cube_transform] = m_cube->add<mesh, transform>();
        cube_mesh->set_geometry(m_geometry.get());
        cube_mesh->add_submesh(
            0,
            m_geometry->get_vertex_count(),
            0,
            m_geometry->get_index_count(),
            m_material.get());

        m_camera = std::make_unique<actor>("main camera", get_world());
        auto [camera_transform, main_camera, camera_control] =
            m_camera->add<transform, camera, orbit_control>();
        camera_transform->set_position(float3{0.0f, 0.0f, -10.0f});

        main_camera->set_skybox(m_skybox.get(), m_skybox_sampler.get());
        main_camera->set_renderer(m_renderer.get());
    }

    void tick(float delta) {}

    void resize(std::uint32_t width, std::uint32_t height)
    {
        render_device& device = render_device::instance();

        m_swapchain->resize(width, height);

        rhi_texture_desc depth_buffer_desc = {};
        depth_buffer_desc.create_type = RHI_TEXTURE_CREATE_FROM_INFO;
        depth_buffer_desc.info.extent.width = width;
        depth_buffer_desc.info.extent.height = height;
        depth_buffer_desc.info.format = RHI_FORMAT_D24_UNORM_S8_UINT;
        depth_buffer_desc.samples = RHI_SAMPLE_COUNT_1;
        depth_buffer_desc.flags = RHI_TEXTURE_DEPTH_STENCIL;
        m_depth_buffer = device.create_texture(depth_buffer_desc);

        rhi_texture_desc light_buffer_desc = {};
        depth_buffer_desc.create_type = RHI_TEXTURE_CREATE_FROM_INFO;
        light_buffer_desc.info.extent.width = width;
        light_buffer_desc.info.extent.height = height;
        light_buffer_desc.info.format = RHI_FORMAT_R8G8B8A8_UNORM;
        light_buffer_desc.samples = RHI_SAMPLE_COUNT_1;
        light_buffer_desc.flags = RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE;

        auto main_camera = m_camera->get<camera>();
        main_camera->resize(width, height);
        main_camera->set_render_target(0, m_swapchain.get());
        main_camera->set_render_target(1, m_depth_buffer.get());
    }

    rhi_ptr<rhi_swapchain> m_swapchain;
    rhi_ptr<rhi_texture> m_depth_buffer;

    rhi_ptr<rhi_texture> m_env_map;
    rhi_ptr<rhi_texture> m_irradiance_map;
    rhi_ptr<rhi_texture> m_prefilter_map;
    rhi_ptr<rhi_texture> m_brdf_lut;
    rhi_ptr<rhi_texture> m_skybox;
    rhi_ptr<rhi_sampler> m_skybox_sampler;

    std::unique_ptr<geometry> m_geometry;
    std::unique_ptr<test_material> m_material;
    std::unique_ptr<actor> m_cube;

    std::unique_ptr<actor> m_light;
    std::unique_ptr<actor> m_camera;

    std::unique_ptr<renderer> m_renderer;

    std::string m_skybox_path;
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