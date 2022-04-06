#include "animation.hpp"
#include "application.hpp"
#include "geometry.hpp"
#include "graphics.hpp"
#include "log.hpp"
#include "mmd_viewer.hpp"
#include "physics.hpp"
#include "pmx_loader.hpp"
#include "scene.hpp"
#include "window.hpp"

using namespace ash::core;
using namespace ash::graphics;
using namespace ash::window;
using namespace ash::ecs;
using namespace ash::scene;
using namespace ash::physics;
using namespace ash::sample::mmd;

namespace ash::sample::mmd
{
struct vertex
{
    math::float3 position;
    math::float3 normal;
    math::float2 uv;

    math::uint4 bone;
    math::float3 bone_weight;
};

class test_module : public submodule
{
public:
    static constexpr ash::uuid id = "bd58a298-9ea4-4f8d-a79c-e57ae694915a";

public:
    test_module(application* app) : submodule("test_module"), m_app(app) {}

    virtual bool initialize(const ash::dictionary& config) override
    {
        initialize_resource();
        initialize_plane();
        initialize_camera();
        initialize_task();

        return true;
    }

private:
    void initialize_resource()
    {
        auto& world = module<ash::ecs::world>();

        m_actor = module<mmd_viewer>().load_mmd("sora", "resource/model/sora/Sora.pmx");

        auto actor_transform = world.component<transform>(m_actor);
        actor_transform->node()->parent(module<ash::scene::scene>().root_node());
    }

    void initialize_plane()
    {
        collision_shape_desc desc;
        desc.type = collision_shape_type::BOX;
        desc.box.length = 1000.0f;
        desc.box.height = 0.5f;
        desc.box.width = 1000.0f;
        m_plane_shape = module<ash::physics::physics>().make_shape(desc);

        ash::ecs::world& world = module<ash::ecs::world>();
        m_plane = world.create();

        world.add<rigidbody, transform>(m_plane);

        auto t = world.component<transform>(m_plane);
        t->position(0.0f, -3.0f, 0.0f);
        t->node()->parent(module<ash::scene::scene>().root_node());

        auto r = world.component<rigidbody>(m_plane);
        r->shape(m_plane_shape.get());
        r->mass(0.0f);
    }

    void initialize_camera()
    {
        ash::ecs::world& world = module<ash::ecs::world>();

        m_camera = world.create();
        world.add<main_camera, camera, transform>(m_camera);
        auto c_camera = world.component<camera>(m_camera);
        c_camera->set(math::to_radians(30.0f), 1300.0f / 800.0f, 0.01f, 1000.0f);

        auto c_transform = world.component<transform>(m_camera);
        c_transform->position(0.0f, 16.0f, -38.0f);
        c_transform->rotation(0.0f, 0.0f, 0.0f, 1.0f);
        c_transform->scaling(1.0f, 1.0f, 1.0f);
        c_transform->node()->parent(module<ash::scene::scene>().root_node());
    }

    void initialize_task()
    {
        auto& task = module<task::task_manager>();

        auto update_task = task.schedule("test update", [this]() { update(); });

        auto window_task = task.find(ash::window::window::TASK_WINDOW_TICK);
        auto render_task = task.find(ash::graphics::graphics::TASK_RENDER);
        auto physics_task = task.find(ash::physics::physics::TASK_SIMULATION);

        window_task->add_dependency(*task.find("root"));
        physics_task->add_dependency(*window_task);
        update_task->add_dependency(*physics_task);
        render_task->add_dependency(*update_task);
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

            auto v = world.component<visual>(m_actor);
            v->submesh[0].parameters[1]->set(0, colors[index]);

            index = (index + 1) % colors.size();
        }

        auto camera_transform = world.component<transform>(m_camera);
        if (mouse.mode() == mouse_mode::CURSOR_RELATIVE)
        {
            m_heading += mouse.x() * m_rotate_speed * delta;
            m_pitch += mouse.y() * m_rotate_speed * delta;
            m_pitch = std::clamp(m_pitch, -math::PI_PIDIV2, math::PI_PIDIV2);
            camera_transform->rotation(
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

        math::float4_simd s = math::simd::load(camera_transform->scaling());
        math::float4_simd r = math::simd::load(camera_transform->rotation());
        math::float4_simd t = math::simd::load(camera_transform->position());

        math::float4x4_simd affine = math::matrix_simd::affine_transform(s, r, t);
        math::float4_simd forward =
            math::simd::set(x * m_move_speed * delta, 0.0f, z * m_move_speed * delta, 0.0f);
        forward = math::matrix_simd::mul(forward, affine);
        camera_transform->position(math::vector_simd::add(forward, t));

        if (keyboard.key(keyboard_key::KEY_T).down())
        {
        }
    }

    void update_actor(float delta)
    {
        auto& world = module<ash::ecs::world>();
        auto& keyboard = module<ash::window::window>().keyboard();

        float move = 0.0f;
        if (keyboard.key(keyboard_key::KEY_E).down())
            move += 1.0f;
        if (keyboard.key(keyboard_key::KEY_Q).down())
            move -= 1.0f;

        auto actor_transform = world.component<transform>(m_actor);
        math::float3 new_position = actor_transform->position();
        new_position[0] += move * m_move_speed * delta;
        actor_transform->position(new_position);
    }

    void update()
    {
        if (module<ash::window::window>().keyboard().key(keyboard_key::KEY_ESC).down())
            m_app->exit();

        float delta = module<ash::core::timer>().frame_delta();
        update_camera(delta);
        update_actor(delta);

        module<animation>().update();
    }

    std::string m_title;
    application* m_app;

    entity m_camera;
    entity m_actor;
    entity m_plane;

    std::unique_ptr<collision_shape_interface> m_plane_shape;

    float m_heading = 0.0f, m_pitch = 0.0f;

    float m_rotate_speed = 0.2f;
    float m_move_speed = 7.0f;
};
} // namespace ash::sample::mmd

int main()
{
    application app;
    app.install<window>();
    app.install<scene>();
    app.install<graphics>();
    app.install<physics>();
    app.install<animation>();
    app.install<mmd_viewer>();
    app.install<test_module>(&app);

    app.run();

    return 0;
}