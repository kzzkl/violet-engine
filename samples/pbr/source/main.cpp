#include "common/log.hpp"
#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/mesh.hpp"
#include "components/orbit_control.hpp"
#include "components/transform.hpp"
#include "control/control_system.hpp"
#include "core/engine.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/geometries/sphere_geometry.hpp"
#include "graphics/geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "pbr_render.hpp"
#include "scene/scene_system.hpp"
#include "task/task_system.hpp"
#include "window/window_system.hpp"

namespace violet::sample
{
class pbr_sample : public engine_system
{
public:
    pbr_sample() : engine_system("pbr sample"), m_material(nullptr) {}

    bool initialize(const dictionary& config) override
    {
        get_system<task_system>().on_tick().then(
            [this](float delta)
            {
                tick(delta);
            });

        auto& window = get_system<window_system>();
        window.on_resize().then(
            [this](std::uint32_t width, std::uint32_t height)
            {
                resize(width, height);
            });

        renderer* renderer = get_system<graphics_system>().get_renderer();

        m_camera = std::make_unique<actor>("main camera", get_world());
        auto [camera_transform, main_camera, camera_control] =
            m_camera->add<transform, camera, orbit_control>();

        m_skybox = renderer->create_image_cube(
            "pbr/skybox/icebergs/right.jpg",
            "pbr/skybox/icebergs/left.jpg",
            "pbr/skybox/icebergs/top.jpg",
            "pbr/skybox/icebergs/bottom.jpg",
            "pbr/skybox/icebergs/front.jpg",
            "pbr/skybox/icebergs/back.jpg");

        rhi_sampler_desc sampler_desc = {};
        sampler_desc.min_filter = RHI_FILTER_LINEAR;
        sampler_desc.mag_filter = RHI_FILTER_LINEAR;
        sampler_desc.address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_desc.address_mode_v = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_desc.address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        m_skybox_sampler = renderer->create_sampler(sampler_desc);

        main_camera->set_skybox(m_skybox.get(), m_skybox_sampler.get());

        m_render_graph = std::make_unique<pbr_render_graph>(renderer);
        m_render_graph->set_camera("main camera", main_camera->get_parameter());

        /*m_irradiance_map = renderer->create_image(rhi_image_desc{
            32,
            32,
            RHI_RESOURCE_FORMAT_R8G8B8A8_UNORM,
            RHI_SAMPLE_COUNT_1,
            RHI_IMAGE_FLAG_RENDER_TARGET | RHI_IMAGE_FLAG_TRANSFER_SRC});

        m_preprocess_graph = std::make_unique<preprocess_graph>(renderer);
        m_preprocess_graph->set_camera("main camera", main_camera->get_parameter());
        m_preprocess_graph->set_target(m_skybox.get(), m_irradiance_map.get());
        get_system<graphics_system>().render(m_preprocess_graph.get());*/

        auto extent = window.get_extent();
        resize(extent.width, extent.height);

        m_geometry = std::make_unique<sphere_geometry>(renderer);

        m_cube = std::make_unique<actor>("cube", get_world());
        auto [cube_transform, cube_mesh] = m_cube->add<transform, mesh>();
        cube_mesh->set_geometry(m_geometry.get());

        m_material = m_render_graph->add_material<pbr_material>("test pbr");
        m_material->set_albedo(float3{0.5f, 0.5f, 0.5f});
        m_material->set_metalness(0.2f);
        m_material->set_roughness(m_roughness);
        cube_mesh->add_submesh(0, 0, 0, m_geometry->get_index_count(), m_material);

        m_light = std::make_unique<actor>("light", get_world());
        auto [light_transform, main_light] = m_light->add<transform, light>();
        light_transform->set_position(1.0f, 1.0f, 1.0f);
        light_transform->lookat(float3{0.0f, 0.0f, 0.0f}, float3{0.0f, 1.0f, 0.0f});

        main_light->color = {1.0f, 1.0f, 1.0f};
        main_light->type = LIGHT_TYPE_DIRECTIONAL;

        return true;
    }

private:
    void tick(float delta)
    {
        auto& window = get_system<window_system>();
        if (window.get_keyboard().key(KEYBOARD_KEY_ADD).press())
        {
            m_roughness = std::min(1.0f, m_roughness + delta * 10.0f);
            m_material->set_roughness(m_roughness);
            log::debug("{}", m_roughness);

            m_camera->get<camera>()->set_skybox(m_irradiance_map.get(), m_skybox_sampler.get());
        }
        if (window.get_keyboard().key(KEYBOARD_KEY_SUBTRACT).press())
        {
            m_roughness = std::max(0.0f, m_roughness - delta * 10.0f);
            m_material->set_roughness(m_roughness);
            log::debug("{}", m_roughness);
        }

        get_system<graphics_system>().render(m_render_graph.get());
    }

    void resize(std::uint32_t width, std::uint32_t height)
    {
        rhi_image_desc depth_stencil_desc = {};
        depth_stencil_desc.width = width;
        depth_stencil_desc.height = height;
        depth_stencil_desc.samples = RHI_SAMPLE_COUNT_1;
        depth_stencil_desc.format = RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT;
        depth_stencil_desc.flags = RHI_IMAGE_FLAG_DEPTH_STENCIL;
        m_depth_stencil =
            get_system<graphics_system>().get_renderer()->create_image(depth_stencil_desc);

        auto main_camera = m_camera->get<camera>();
        main_camera->resize(width, height);
        // main_camera->set_attachment(0, nullptr, true);
        // main_camera->set_attachment(1, m_depth_stencil.get());

        m_render_graph->get_slot("depth buffer")->set_image(m_depth_stencil.get());
        // m_render_graph->get_camera("main camera")->set_parameter(main_camera->get_parameter());
    }

    std::unique_ptr<preprocess_graph> m_preprocess_graph;
    rhi_ptr<rhi_image> m_irradiance_map;

    std::unique_ptr<pbr_render_graph> m_render_graph;
    rhi_ptr<rhi_image> m_depth_stencil;

    rhi_ptr<rhi_image> m_skybox;
    rhi_ptr<rhi_sampler> m_skybox_sampler;

    pbr_material* m_material;
    float m_roughness = 0.3f;

    std::unique_ptr<geometry> m_geometry;
    std::unique_ptr<actor> m_cube;

    std::unique_ptr<actor> m_light;

    std::unique_ptr<actor> m_camera;
};
} // namespace violet::sample

int main()
{
    using namespace violet;

    engine engine;
    engine.initialize("");
    engine.install<task_system>();
    engine.install<scene_system>();
    engine.install<window_system>();
    engine.install<graphics_system>();
    engine.install<control_system>();
    engine.install<sample::pbr_sample>();

    engine.get_system<window_system>().on_destroy().then(
        [&engine]()
        {
            engine.exit();
        });

    engine.run();

    return 0;
}