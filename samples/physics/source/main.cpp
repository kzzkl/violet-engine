#include "application.hpp"
#include "geometry.hpp"
#include "graphics.hpp"
#include "log.hpp"
#include "physics.hpp"
#include "scene.hpp"
#include "window.hpp"

using namespace ash::core;
using namespace ash::graphics;
using namespace ash::window;
using namespace ash::ecs;
using namespace ash::scene;
using namespace ash::physics;

namespace ash::sample::physics
{
class test_module : public submodule
{
public:
    static constexpr ash::uuid id = "bd58a298-9ea4-4f8d-a79c-e57ae694915a";

public:
    test_module(application* app) : submodule("test_module") {}

    virtual bool initialize(const ash::dictionary& config) override
    {
        ash::ecs::world& world = module<ash::ecs::world>();

        // Create cube.
        m_cube = world.create();
        world.add<rigidbody, transform, visual>(m_cube);

        transform& t = world.component<transform>(m_cube);
        t.position(1.0f, 0.0f, 0.0f);
        t.node()->parent(module<ash::scene::scene>().root_node());

        rigidbody& r = world.component<rigidbody>(m_cube);
        m_shape = module<ash::physics::physics>().make_shape();
        r.shape = m_shape.get();

        geometry_data cube_data = geometry::box(1.0f, 1.0f, 1.0f);
        m_pipeline = module<ash::graphics::graphics>().make_render_pipeline("geometry");
        visual& v = world.component<visual>(m_cube);
        v.vertex_buffer = module<ash::graphics::graphics>().make_vertex_buffer(
            cube_data.vertices.data(),
            cube_data.vertices.size());
        v.index_buffer = module<ash::graphics::graphics>().make_index_buffer(
            cube_data.indices.data(),
            cube_data.indices.size());
        v.submesh.push_back({0, cube_data.indices.size()});
        v.material.resize(1);
        v.material[0].pipeline = m_pipeline.get();
        v.material[0].property =
            module<ash::graphics::graphics>().make_render_parameter("geometry_material");
        v.material[0].property->set(0, math::float4{1.0f, 1.0f, 1.0f, 1.0f});
        v.object = module<ash::graphics::graphics>().make_render_parameter("ash_object");

        // Create plane.
        m_plane = world.create();
        world.add<rigidbody, transform>(m_plane);

        transform& pt = world.component<transform>(m_plane);
        pt.node()->parent(module<ash::scene::scene>().root_node());

        rigidbody& pr = world.component<rigidbody>(m_plane);
        pr.shape = m_shape.get();

        initialize_task();
        initialize_camera();

        return true;
    }

private:
    void initialize_task()
    {
        auto& task = module<ash::task::task_manager>();

        auto update_task = task.schedule("test update", [this]() { update(); });

        auto window_task = task.find(ash::window::window::TASK_WINDOW_TICK);
        auto render_task = task.find(ash::graphics::graphics::TASK_RENDER);
        auto physics_task = task.find(ash::physics::physics::TASK_SIMULATION);

        window_task->add_dependency(*task.find("root"));
        physics_task->add_dependency(*window_task);
        update_task->add_dependency(*physics_task);
        render_task->add_dependency(*update_task);
    }

    void initialize_camera()
    {
        ash::ecs::world& world = module<ash::ecs::world>();

        m_camera = world.create();
        world.add<main_camera, camera, transform>(m_camera);
        camera& c_camera = world.component<camera>(m_camera);
        c_camera.set(math::to_radians(30.0f), 1300.0f / 800.0f, 0.01f, 1000.0f);

        transform& c_transform = world.component<transform>(m_camera);
        c_transform.position(0.0f, 0.0f, -38.0f);
        c_transform.node()->parent(module<ash::scene::scene>().root_node());
        c_transform.node()->to_world = math::matrix_plain::affine_transform(
            c_transform.scaling(),
            c_transform.rotation(),
            c_transform.position());
    }

    void update()
    {
        float delta = module<ash::core::timer>().frame_delta();
        update_camera(delta);

        module<ash::graphics::graphics>().debug().draw_line(
            {100.0f, 0.0f, 0.0f},
            {-100.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f});
    }

    void update_camera(float delta)
    {
        auto& world = module<ash::ecs::world>();
        auto& keyboard = module<ash::window::window>().keyboard();
        auto& mouse = module<ash::window::window>().mouse();

        if (keyboard.key(keyboard_key::KEY_1).release())
        {
            if (mouse.mode() == mouse_mode::CURSOR_RELATIVE)
                mouse.mode(mouse_mode::CURSOR_ABSOLUTE);
            else
                mouse.mode(mouse_mode::CURSOR_RELATIVE);
        }

        if (keyboard.key(keyboard_key::KEY_3).release())
        {
            static std::size_t index = 0;
            static std::vector<math::float4> colors = {
                math::float4{1.0f, 0.0f, 0.0f, 1.0f},
                math::float4{0.0f, 1.0f, 0.0f, 1.0f},
                math::float4{0.0f, 0.0f, 1.0f, 1.0f}
            };

            visual& v = world.component<visual>(m_cube);
            v.material[0].property->set(0, colors[index]);

            index = (index + 1) % colors.size();
        }

        transform& camera_transform = world.component<transform>(m_camera);
        if (mouse.mode() == mouse_mode::CURSOR_RELATIVE)
        {
            m_heading += mouse.x() * m_rotate_speed * delta;
            m_pitch += mouse.y() * m_rotate_speed * delta;
            m_pitch = std::clamp(m_pitch, -math::PI_PIDIV2, math::PI_PIDIV2);
            camera_transform.rotation(
                math::quaternion_plain::rotation_euler(m_heading, m_pitch, 0.0f));
        }

        float x = 0, z = 0;
        if (keyboard.key(keyboard_key::KEY_W).down())
            z += 1.0f;
        if (keyboard.key(keyboard_key::KEY_S).down())
            z -= 1.0f;
        if (keyboard.key(keyboard_key::KEY_D).down())
            x += 1.0f;
        if (keyboard.key(keyboard_key::KEY_A).down())
            x -= 1.0f;

        math::float4_simd s = math::simd::load(camera_transform.scaling());
        math::float4_simd r = math::simd::load(camera_transform.rotation());
        math::float4_simd t = math::simd::load(camera_transform.position());

        math::float4x4_simd affine = math::matrix_simd::affine_transform(s, r, t);
        math::float4_simd forward =
            math::simd::set(x * m_move_speed * delta, 0.0f, z * m_move_speed * delta, 0.0f);
        forward = math::matrix_simd::mul(forward, affine);

        camera_transform.position(math::vector_simd::add(forward, t));
    }

    std::unique_ptr<collision_shape_interface> m_shape;

    std::unique_ptr<render_pipeline> m_pipeline;

    entity_id m_cube;
    entity_id m_plane;
    entity_id m_camera;

    float m_heading = 0.0f, m_pitch = 0.0f;

    float m_rotate_speed = 0.2f;
    float m_move_speed = 7.0f;
};
} // namespace ash::sample::physics

int main()
{
    application app;
    app.install<window>();
    app.install<scene>();
    app.install<graphics>();
    app.install<physics>();
    app.install<ash::sample::physics::test_module>(&app);

    app.run();

    return 0;
}