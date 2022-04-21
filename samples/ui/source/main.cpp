#include "application.hpp"
#include "assert.hpp"
#include "graphics.hpp"
#include "relation.hpp"
#include "scene.hpp"
#include "ui.hpp"
#include "window.hpp"

namespace ash::sample ::ui
{
class test_system : public core::system_base
{
public:
    test_system() : core::system_base("test") {}

    virtual bool initialize(const dictionary& config) override
    {
        initialize_task();
        initialize_camera();
        return true;
    }

private:
    void initialize_task()
    {
        auto& task = system<task::task_manager>();

        auto update_task = task.schedule("test update", [this]() { update(); });
        auto window_task = task.schedule(
            "window tick",
            [this]() { system<window::window>().tick(); },
            task::task_type::MAIN_THREAD);
        auto render_task =
            task.schedule("render", [this]() { system<graphics::graphics>().render(); });

        window_task->add_dependency(*task.find("root"));
        update_task->add_dependency(*window_task);
        render_task->add_dependency(*update_task);
    }

    void initialize_camera()
    {
        auto& world = system<ash::ecs::world>();
        auto& scene = system<ash::scene::scene>();
        auto& relation = system<ash::core::relation>();

        m_camera = world.create();
        world.add<core::link, graphics::main_camera, graphics::camera, scene::transform>(m_camera);
        auto& c_camera = world.component<graphics::camera>(m_camera);
        c_camera.set(math::to_radians(30.0f), 1300.0f / 800.0f, 0.01f, 1000.0f);

        auto& c_transform = world.component<scene::transform>(m_camera);
        c_transform.position = {0.0f, 0.0f, -38.0f};
        c_transform.world_matrix = math::matrix_plain::affine_transform(
            c_transform.scaling,
            c_transform.rotation,
            c_transform.position);
        c_transform.dirty = true;

        relation.link(m_camera, scene.root());
    }

    void update_camera(float delta)
    {
        auto& world = system<ecs::world>();
        auto& keyboard = system<window::window>().keyboard();
        auto& mouse = system<window::window>().mouse();

        if (keyboard.key(window::keyboard_key::KEY_1).release())
        {
            if (mouse.mode() == window::mouse_mode::CURSOR_RELATIVE)
                mouse.mode(window::mouse_mode::CURSOR_ABSOLUTE);
            else
                mouse.mode(window::mouse_mode::CURSOR_RELATIVE);
            log::debug("{}", mouse.mode());
        }

        auto& camera_transform = world.component<scene::transform>(m_camera);
        if (mouse.mode() == window::mouse_mode::CURSOR_RELATIVE)
        {
            m_heading += mouse.x() * m_rotate_speed * delta;
            m_pitch += mouse.y() * m_rotate_speed * delta;
            m_pitch = std::clamp(m_pitch, -math::PI_PIDIV2, math::PI_PIDIV2);
            camera_transform.rotation =
                math::quaternion_plain::rotation_euler(m_heading, m_pitch, 0.0f);
        }

        float x = 0, z = 0;
        if (keyboard.key(window::keyboard_key::KEY_W).down())
            z += 1.0f;
        if (keyboard.key(window::keyboard_key::KEY_S).down())
            z -= 1.0f;
        if (keyboard.key(window::keyboard_key::KEY_D).down())
            x += 1.0f;
        if (keyboard.key(window::keyboard_key::KEY_A).down())
            x -= 1.0f;

        math::float4_simd s = math::simd::load(camera_transform.scaling);
        math::float4_simd r = math::simd::load(camera_transform.rotation);
        math::float4_simd t = math::simd::load(camera_transform.position);

        math::float4x4_simd affine = math::matrix_simd::affine_transform(s, r, t);
        math::float4_simd forward =
            math::simd::set(x * m_move_speed * delta, 0.0f, z * m_move_speed * delta, 0.0f);
        forward = math::matrix_simd::mul(forward, affine);

        math::simd::store(math::vector_simd::add(forward, t), camera_transform.position);
        camera_transform.dirty = true;
    }

    void update()
    {
        auto& world = system<ash::ecs::world>();
        auto& scene = system<scene::scene>();
        scene.reset_sync_counter();

        bool show = true;
        auto& ui = system<ash::ui::ui>();
        ui.begin_frame();
        ui.end_frame();

        update_camera(system<ash::core::timer>().frame_delta());
        system<scene::scene>().sync_local();
    }

    ecs::entity m_camera;
    float m_heading = 0.0f, m_pitch = 0.0f;

    float m_rotate_speed = 0.2f;
    float m_move_speed = 7.0f;
};

class ui_app
{
public:
    void initialize()
    {
        m_app.install<window::window>();
        m_app.install<core::relation>();
        m_app.install<scene::scene>();
        m_app.install<graphics::graphics>();
        m_app.install<ash::ui::ui>();
        m_app.install<test_system>();
    }

    void run() { m_app.run(); }

private:
    core::application m_app;
};
} // namespace ash::sample::ui

int main()
{
    ash::sample ::ui::ui_app app;
    app.initialize();
    app.run();
    return 0;
}