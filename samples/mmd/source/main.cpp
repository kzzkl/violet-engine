#include "application.hpp"
#include "graphics.hpp"
#include "log.hpp"
#include "pmx_loader.hpp"
#include "scene.hpp"
#include "window.hpp"

using namespace ash::core;
using namespace ash::graphics;
using namespace ash::window;
using namespace ash::ecs;
using namespace ash::scene;
using namespace ash::sample::mmd;

namespace ash::sample::mmd
{
struct vertex
{
    math::float3 position;
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
        initialize_camera();
        initialize_task();

        return true;
    }

private:
    void initialize_resource()
    {
        pmx_loader loader;
        if (!loader.load("resource/White.pmx"))
        {
            ash::log::error("Load pmx failed");
            return;
        }

        std::vector<vertex> vertices;
        vertices.reserve(loader.get_vertices().size());
        for (const pmx_vertex& v : loader.get_vertices())
            vertices.push_back(vertex{v.position});

        std::vector<std::int32_t> indices;
        indices.reserve(loader.get_indices().size());
        for (std::int32_t i : loader.get_indices())
            indices.push_back(i);

        ash::ecs::world& world = module<ash::ecs::world>();
        entity_id entity = world.create();

        world.add<visual, mesh, material>(entity);

        visual& v = world.component<visual>(entity);
        v.group = module<ash::graphics::graphics>().group("mmd");

        mesh& m = world.component<mesh>(entity);
        m.vertex_buffer = module<ash::graphics::graphics>().make_vertex_buffer<vertex>(
            vertices.data(),
            vertices.size());
        m.index_buffer = module<ash::graphics::graphics>().make_index_buffer<std::int32_t>(
            indices.data(),
            indices.size());
    }

    void initialize_camera()
    {
        ash::ecs::world& world = module<ash::ecs::world>();

        m_camera = world.create();
        world.add<main_camera, camera, transform>(m_camera);
        camera& c_camera = world.component<camera>(m_camera);
        c_camera.set(math::to_radians(30.0f), 1300.0f / 800.0f, 0.01f, 1000.0f);

        transform& c_transform = world.component<transform>(m_camera);
        c_transform.position = {0.0f, 16.0f, -38.0f};
        c_transform.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        c_transform.scaling = {1.0f, 1.0f, 1.0f};
        c_transform.node = std::make_unique<scene_node>();
        c_transform.parent = module<ash::scene::scene>().root_node();
    }

    void initialize_task()
    {
        auto& task = module<task::task_manager>();

        auto update_task = task.schedule("test update", [this]() { update(); });
        update_task->add_dependency(*task.find("window"));

        task.find("scene")->add_dependency(*update_task);
    }

    void update_camera()
    {
        auto& world = module<ash::ecs::world>();
        auto& keyboard = module<ash::window::window>().keyboard();
        auto& mouse = module<ash::window::window>().mouse();
        float delta = module<ash::core::timer>().frame_delta();

        if (keyboard.key(keyboard_key::KEY_1).down())
            mouse.mode(mouse_mode::CURSOR_RELATIVE);
        if (keyboard.key(keyboard_key::KEY_2).down())
            mouse.mode(mouse_mode::CURSOR_ABSOLUTE);

        transform& camera_transform = world.component<transform>(m_camera);
        if (mouse.mode() == mouse_mode::CURSOR_RELATIVE)
        {
            m_heading += mouse.x() * m_rotate_speed * delta;
            m_pitch += mouse.y() * m_rotate_speed * delta;
            m_pitch = std::clamp(m_pitch, -math::PI_PIDIV2, math::PI_PIDIV2);
            camera_transform.rotation =
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

        math::float4_simd s = math::simd::load(camera_transform.scaling);
        math::float4_simd r = math::simd::load(camera_transform.rotation);
        math::float4_simd t = math::simd::load(camera_transform.position);

        math::float4x4_simd affine = math::matrix_simd::affine_transform(s, r, t);
        math::float4_simd forward =
            math::simd::set(x * m_move_speed * delta, 0.0f, z * m_move_speed * delta, 0.0f);
        forward = math::matrix_simd::mul(forward, affine);
        math::simd::store(math::vector_simd::add(forward, t), camera_transform.position);

        camera_transform.node->dirty = true;
    }

    void update()
    {
        if (module<ash::window::window>().keyboard().key(keyboard_key::KEY_ESC).down())
            m_app->exit();

        update_camera();
    }

    std::string m_title;
    application* m_app;

    entity_id m_camera;

    float m_heading = 0.0f, m_pitch = 0.0f;

    float m_rotate_speed = 0.2f;
    float m_move_speed = 5.0f;
};
} // namespace ash::sample::mmd

int main()
{
    ash::math::float4_simd t = ash::math::simd::set(1.0, 2.0f, 3.0f, 0.0f);
    ash::math::float4_simd r = ash::math::simd::set(2.0, 2.0f, 3.0f, 3.0f);
    ash::math::float4_simd s = ash::math::simd::set(1.0, 2.0f, 3.0f, 0.0f);

    ash::math::float4x4_simd result = ash::math::matrix_simd::affine_transform(t, r, s);

    application app;
    app.install<window>();
    app.install<scene>();
    app.install<graphics>();
    app.install<ash::sample::mmd::test_module>(&app);

    app.run();

    return 0;
}