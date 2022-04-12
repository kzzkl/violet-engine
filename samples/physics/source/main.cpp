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
class test_module : public system_base
{
public:
    test_module(application* app) : system_base("test_module") {}

    virtual bool initialize(const ash::dictionary& config) override
    {
        auto& world = system<ash::ecs::world>();
        auto& graphics = system<ash::graphics::graphics>();
        auto& scene = system<ash::scene::scene>();

        // Create rigidbody shape.
        collision_shape_desc desc;
        desc.type = collision_shape_type::BOX;
        desc.box.length = 1.0f;
        desc.box.height = 1.0f;
        desc.box.width = 1.0f;
        m_cube_shape = system<ash::physics::physics>().make_shape(desc);

        desc.type = collision_shape_type::BOX;
        desc.box.length = 10.0f;
        desc.box.height = 0.05f;
        desc.box.width = 10.0f;
        m_plane_shape = system<ash::physics::physics>().make_shape(desc);

        geometry_data cube_data = geometry::box(1.0f, 1.0f, 1.0f);
        m_cube_vertex_buffer =
            graphics.make_vertex_buffer(cube_data.vertices.data(), cube_data.vertices.size());
        m_cube_index_buffer =
            graphics.make_index_buffer(cube_data.indices.data(), cube_data.indices.size());
        m_cube_material = graphics.make_render_parameter("geometry_material");
        m_cube_material->set(0, math::float4{1.0f, 1.0f, 1.0f, 1.0f});
        m_pipeline = graphics.make_render_pipeline<render_pipeline>("geometry");

        // Create cube.
        {
            m_cube_1 = world.create();
            world.add<rigidbody, transform, visual>(m_cube_1);

            auto t = world.component<transform>(m_cube_1);
            t->position = {1.0f, 0.0f, 0.0f};
            t->rotation = math::quaternion_plain::rotation_euler(1.0f, 1.0f, 0.5f);
            scene.link(*t);

            auto r = world.component<rigidbody>(m_cube_1);
            r->shape(m_cube_shape.get());
            r->mass(1.0f);

            auto v = world.component<visual>(m_cube_1);
            m_cube_object.emplace_back(graphics.make_render_parameter("ash_object"));
            v->object = m_cube_object.back().get();

            render_unit submesh;
            submesh.vertex_buffer = m_cube_vertex_buffer.get();
            submesh.index_buffer = m_cube_index_buffer.get();
            submesh.index_start = 0;
            submesh.index_end = cube_data.indices.size();
            submesh.pipeline = m_pipeline.get();
            submesh.parameters = {v->object, m_cube_material.get()};
            v->submesh.push_back(submesh);
        }

        // Cube 2.
        {
            m_cube_2 = world.create();
            world.add<rigidbody, transform, visual>(m_cube_2);

            auto t = world.component<transform>(m_cube_2);
            t->position = {-1.0f, 0.0f, 0.0f};
            t->rotation = math::quaternion_plain::rotation_euler(1.0f, 1.0f, 0.5f);
            scene.link(*t, *world.component<scene::transform>(m_cube_1));

            auto r = world.component<rigidbody>(m_cube_2);
            r->shape(m_cube_shape.get());
            r->mass(1.0f);

            auto v = world.component<visual>(m_cube_2);
            m_cube_object.emplace_back(graphics.make_render_parameter("ash_object"));
            v->object = m_cube_object.back().get();

            render_unit submesh;
            submesh.vertex_buffer = m_cube_vertex_buffer.get();
            submesh.index_buffer = m_cube_index_buffer.get();
            submesh.index_start = 0;
            submesh.index_end = cube_data.indices.size();
            submesh.pipeline = m_pipeline.get();
            submesh.parameters = {v->object, m_cube_material.get()};
            v->submesh.push_back(submesh);
        }

        // Create plane.
        m_plane = world.create();
        world.add<rigidbody, transform>(m_plane);

        auto pt = world.component<transform>(m_plane);
        pt->position = {0.0f, -3.0f, 0.0f};
        scene.link(*pt);

        auto pr = world.component<rigidbody>(m_plane);
        pr->shape(m_plane_shape.get());
        pr->mass(0.0f);

        initialize_task();
        initialize_camera();

        return true;
    }

private:
    void initialize_task()
    {
        auto& task = system<ash::task::task_manager>();

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
        auto& world = system<ash::ecs::world>();
        auto& scene = system<ash::scene::scene>();

        m_camera = world.create();
        world.add<main_camera, camera, transform>(m_camera);
        auto c_camera = world.component<camera>(m_camera);
        c_camera->set(math::to_radians(30.0f), 1300.0f / 800.0f, 0.01f, 1000.0f);

        auto c_transform = world.component<transform>(m_camera);
        c_transform->position = {0.0f, 0.0f, -38.0f};
        scene.link(*c_transform);
        c_transform->world_matrix = math::matrix_plain::affine_transform(
            c_transform->scaling,
            c_transform->rotation,
            c_transform->position);
    }

    void update()
    {
        float delta = system<ash::core::timer>().frame_delta();
        update_camera(delta);

        system<ash::graphics::graphics>().debug().draw_line(
            {100.0f, 0.0f, 0.0f},
            {-100.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f});
    }

    void update_camera(float delta)
    {
        auto& world = system<ash::ecs::world>();
        auto& keyboard = system<ash::window::window>().keyboard();
        auto& mouse = system<ash::window::window>().mouse();

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

            m_cube_material->set(0, colors[index]);
            index = (index + 1) % colors.size();
        }

        auto camera_transform = world.component<transform>(m_camera);
        if (mouse.mode() == mouse_mode::CURSOR_RELATIVE)
        {
            m_heading += mouse.x() * m_rotate_speed * delta;
            m_pitch += mouse.y() * m_rotate_speed * delta;
            m_pitch = std::clamp(m_pitch, -math::PI_PIDIV2, math::PI_PIDIV2);
            camera_transform->rotation =
                math::quaternion_plain::rotation_euler(m_heading, m_pitch, 0.0f);
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

        math::float4_simd s = math::simd::load(camera_transform->scaling);
        math::float4_simd r = math::simd::load(camera_transform->rotation);
        math::float4_simd t = math::simd::load(camera_transform->position);

        math::float4x4_simd affine = math::matrix_simd::affine_transform(s, r, t);
        math::float4_simd forward =
            math::simd::set(x * m_move_speed * delta, 0.0f, z * m_move_speed * delta, 0.0f);
        forward = math::matrix_simd::mul(forward, affine);

        math::simd::store(math::vector_simd::add(forward, t), camera_transform->position);
    }

    std::unique_ptr<collision_shape_interface> m_cube_shape;
    std::unique_ptr<collision_shape_interface> m_plane_shape;

    std::unique_ptr<render_pipeline> m_pipeline;

    std::unique_ptr<graphics::resource> m_cube_vertex_buffer;
    std::unique_ptr<graphics::resource> m_cube_index_buffer;
    std::unique_ptr<graphics::render_parameter> m_cube_material;
    std::vector<std::unique_ptr<graphics::render_parameter>> m_cube_object;

    entity m_cube_1, m_cube_2;
    entity m_plane;
    entity m_camera;

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