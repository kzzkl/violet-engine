#include "core/application.hpp"
#include "core/relation.hpp"
#include "core/timer.hpp"
#include "graphics/blinn_phong_pipeline.hpp"
#include "graphics/camera.hpp"
#include "graphics/geometry.hpp"
#include "graphics/graphics.hpp"
#include "graphics/rhi.hpp"
#include "mmd_animation.hpp"
#include "mmd_viewer.hpp"
#include "physics/physics.hpp"
#include "pmx_loader.hpp"
#include "scene/scene.hpp"
#include "task/task_manager.hpp"
#include "window/window.hpp"
#include "window/window_event.hpp"

#define EDITOR_MODE

#if defined(EDITOR_MODE)
#    include "editor/editor.hpp"
#    include "ui/ui.hpp"
#endif

namespace ash::sample::mmd
{
class sample_module : public core::system_base
{
public:
    sample_module(core::application* app) : core::system_base("sample_module"), m_app(app) {}

    virtual bool initialize(const dictionary& config) override
    {
        initialize_resource();
        initialize_scene();
        initialize_camera();
        initialize_task();

        system<core::event>().subscribe<window::event_window_resize>(
            "sample_module",
            [this](std::uint32_t width, std::uint32_t height) { resize_camera(width, height); });

        return true;
    }

    virtual void shutdown() override
    {
        auto& world = system<ecs::world>();
        world.release(m_actor);
        world.release(m_plane);
        world.release(m_camera);
    }

private:
    void initialize_resource()
    {
        auto& world = system<ecs::world>();
        auto& scene = system<scene::scene>();
        auto& relation = system<core::relation>();

        m_actor = system<mmd_viewer>().load_mmd(
            "sora",
            "resource/model/sora/Sora.pmx",
            "resource/model/sora/test.vmd");

        relation.link(m_actor, scene.root());
    }

    void initialize_scene()
    {
        auto& relation = system<core::relation>();
        auto& scene = system<scene::scene>();
        auto& world = system<ecs::world>();

        auto plane_mesh_data = graphics::geometry::box(1.0f, 1.0f, 1.0f);
        m_plane_positon_buffer = graphics::rhi::make_vertex_buffer(
            plane_mesh_data.position.data(),
            plane_mesh_data.position.size());
        m_plane_normal_buffer = graphics::rhi::make_vertex_buffer(
            plane_mesh_data.normal.data(),
            plane_mesh_data.normal.size());
        m_plane_index_buffer = graphics::rhi::make_index_buffer(
            plane_mesh_data.indices.data(),
            plane_mesh_data.indices.size());
        m_plane_material = std::make_unique<graphics::blinn_phong_material_pipeline_parameter>();
        m_plane_material->diffuse(math::float3{1.0f, 1.0f, 1.0f});
        m_plane_material->fresnel(math::float3{0.01f, 0.01f, 0.01f});
        m_plane_material->roughness(0.2f);
        m_blinn_phong_pipeline = std::make_unique<graphics::blinn_phong_pipeline>();

        m_plane = world.create();
        world.add<core::link, graphics::mesh_render, scene::transform, scene::bounding_box>(
            m_plane);

        auto& transform = world.component<scene::transform>(m_plane);
        transform.position(math::float3{0.0f, -0.25f, 0.0f});
        transform.scale(math::float3{100.0f, 0.5f, 100.0f});

        auto& mesh_render = world.component<graphics::mesh_render>(m_plane);
        mesh_render.vertex_buffers = {m_plane_positon_buffer.get(), m_plane_normal_buffer.get()};
        mesh_render.index_buffer = m_plane_index_buffer.get();
        mesh_render.object_parameter = std::make_unique<graphics::object_pipeline_parameter>();

        graphics::material material = {};
        material.pipeline = m_blinn_phong_pipeline.get();
        material.parameters = {
            mesh_render.object_parameter->interface(),
            m_plane_material->interface()};
        mesh_render.materials.push_back(material);
        mesh_render.submeshes.push_back(graphics::submesh{0, 36, 0});

        relation.link(m_plane, scene.root());

        m_light = world.create("light");
        world.add<scene::transform, core::link, graphics::directional_light>(m_light);
        auto& directional_light = world.component<graphics::directional_light>(m_light);
        directional_light.color(math::float3{0.5f, 0.5f, 0.5f});
        relation.link(m_light, scene.root());
    }

    void initialize_camera()
    {
        auto& world = system<ecs::world>();
        auto& scene = system<scene::scene>();
        auto& graphics = system<graphics::graphics>();

        m_camera = world.create("main camera");
        world.add<core::link, graphics::camera, scene::transform>(m_camera);

        auto& transform = world.component<scene::transform>(m_camera);
        transform.position(math::float3{0.0f, 11.0f, -30.0f});
        system<core::relation>().link(m_camera, scene.root());

        auto extent = graphics.render_extent();
        resize_camera(extent.width, extent.height);

        graphics.game_camera(m_camera);
    }

    void initialize_task()
    {
        auto& task = system<task::task_manager>();

        auto update_task = task.schedule("test update", [this]() { update(); });

        update_task->add_dependency(*task.find(task::TASK_GAME_LOGIC_START));
        task.find(task::TASK_GAME_LOGIC_END)->add_dependency(*update_task);
    }

    void resize_camera(std::uint32_t width, std::uint32_t height)
    {
        auto& world = system<ecs::world>();
        auto& camera = world.component<graphics::camera>(m_camera);

        graphics::render_target_info render_target_info = {};
        render_target_info.width = width;
        render_target_info.height = height;
        render_target_info.format = graphics::rhi::back_buffer_format();
        render_target_info.samples = 4;
        m_render_target = graphics::rhi::make_render_target(render_target_info);
        camera.render_target(m_render_target.get());

        graphics::depth_stencil_buffer_info depth_stencil_buffer_info = {};
        depth_stencil_buffer_info.width = width;
        depth_stencil_buffer_info.height = height;
        depth_stencil_buffer_info.format = graphics::RESOURCE_FORMAT_D24_UNORM_S8_UINT;
        depth_stencil_buffer_info.samples = 4;
        m_depth_stencil_buffer =
            graphics::rhi::make_depth_stencil_buffer(depth_stencil_buffer_info);
        camera.depth_stencil_buffer(m_depth_stencil_buffer.get());
    }

    void update_camera(float delta)
    {
        auto& world = system<ecs::world>();
        auto& keyboard = system<window::window>().keyboard();
        auto& mouse = system<window::window>().mouse();

        if (keyboard.key(window::KEYBOARD_KEY_1).release())
        {
            if (mouse.mode() == window::MOUSE_MODE_RELATIVE)
                mouse.mode(window::MOUSE_MODE_ABSOLUTE);
            else
                mouse.mode(window::MOUSE_MODE_RELATIVE);
        }

        auto& camera_transform = world.component<scene::transform>(m_camera);
        if (mouse.mode() == window::MOUSE_MODE_RELATIVE)
        {
            m_heading += mouse.x() * m_rotate_speed * delta;
            m_pitch += mouse.y() * m_rotate_speed * delta;
            m_pitch = std::clamp(m_pitch, -math::PI_PIDIV2, math::PI_PIDIV2);
            camera_transform.rotation_euler(math::float3{m_heading, m_pitch, 0.0f});
        }

        float x = 0, z = 0;
        if (keyboard.key(window::KEYBOARD_KEY_W).down())
            z += 1.0f;
        if (keyboard.key(window::KEYBOARD_KEY_S).down())
            z -= 1.0f;
        if (keyboard.key(window::KEYBOARD_KEY_D).down())
            x += 1.0f;
        if (keyboard.key(window::KEYBOARD_KEY_A).down())
            x -= 1.0f;

        math::float4_simd s = math::simd::load(camera_transform.scale());
        math::float4_simd r = math::simd::load(camera_transform.rotation());
        math::float4_simd t = math::simd::load(camera_transform.position());

        math::float4x4_simd affine = math::matrix_simd::affine_transform(s, r, t);
        math::float4_simd forward =
            math::simd::set(x * m_move_speed * delta, 0.0f, z * m_move_speed * delta, 0.0f);
        forward = math::matrix_simd::mul(forward, affine);
        camera_transform.position(math::vector_simd::add(forward, t));
    }

    void update_actor(float delta)
    {
        auto& world = system<ecs::world>();
        auto& keyboard = system<window::window>().keyboard();

        float move = 0.0f;
        if (keyboard.key(window::KEYBOARD_KEY_E).down())
            move += 1.0f;
        if (keyboard.key(window::KEYBOARD_KEY_Q).down())
            move -= 1.0f;

        if (move != 0.0f)
        {
            auto& actor_transform = world.component<scene::transform>(m_actor);
            math::float3 position = actor_transform.position();
            position[0] += move * m_move_speed * delta;
            actor_transform.position(position);
        }
    }

    void update()
    {
        if (system<window::window>().keyboard().key(window::KEYBOARD_KEY_ESCAPE).down())
            m_app->exit();

        float delta = system<core::timer>().frame_delta();
        update_camera(delta);
        update_actor(delta);

        system<scene::scene>().sync_local();
        system<mmd_viewer>().update();
    }

    std::string m_title;
    core::application* m_app;

    ecs::entity m_actor;
    ecs::entity m_plane;
    ecs::entity m_light;

    ecs::entity m_camera;
    std::unique_ptr<graphics::resource_interface> m_render_target;
    std::unique_ptr<graphics::resource_interface> m_depth_stencil_buffer;

    std::unique_ptr<graphics::resource_interface> m_plane_positon_buffer;
    std::unique_ptr<graphics::resource_interface> m_plane_normal_buffer;
    std::unique_ptr<graphics::resource_interface> m_plane_index_buffer;
    std::unique_ptr<graphics::blinn_phong_material_pipeline_parameter> m_plane_material;
    std::unique_ptr<graphics::blinn_phong_pipeline> m_blinn_phong_pipeline;

    float m_heading = 0.0f, m_pitch = 0.0f;

    float m_rotate_speed = 0.2f;
    float m_move_speed = 7.0f;
};

class mmd_viewer_app
{
public:
    mmd_viewer_app() : m_app("resource/config")
    {
        m_app.install<window::window>();
        m_app.install<core::relation>();
        m_app.install<scene::scene>();
        m_app.install<graphics::graphics>();
        m_app.install<physics::physics>();
        m_app.install<mmd_animation>();
        m_app.install<mmd_viewer>();

#if defined(EDITOR_MODE)
        m_app.install<ui::ui>();
        m_app.install<editor::editor>();
#endif

        m_app.install<sample_module>(&m_app);
    }

    void run()
    {
        m_app.run();
    }

private:
    core::application m_app;
};
} // namespace ash::sample::mmd

int main()
{
    ash::sample::mmd::mmd_viewer_app app;
    app.run();

    return 0;
}