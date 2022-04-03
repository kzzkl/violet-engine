#include "application.hpp"
#include "graphics.hpp"
#include "log.hpp"
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
        initialize_camera();
        initialize_task();

        return true;
    }

private:
    void initialize_resource()
    {
        std::string model_path = "resource/model/sora/";

        pmx_loader loader;
        if (!loader.load(model_path + "Sora.pmx"))
        {
            ash::log::error("Load pmx failed");
            return;
        }

        std::vector<vertex> vertices;
        vertices.reserve(loader.vertices().size());
        for (const pmx_vertex& v : loader.vertices())
            vertices.push_back(vertex{v.position, v.normal, v.uv, v.bone, v.weight});

        std::vector<std::int32_t> indices;
        indices.reserve(loader.indices().size());
        for (std::int32_t i : loader.indices())
            indices.push_back(i);

        ash::ecs::world& world = module<ash::ecs::world>();
        m_actor = world.create();
        world.add<visual, transform>(m_actor);

        auto& graphics = module<ash::graphics::graphics>();

        for (auto& png_path : loader.textures())
        {
            std::string dds_path = png_path.substr(0, png_path.find_last_of('.')) + ".dds";
            m_textures.push_back(graphics.make_texture(model_path + dds_path));
        }

        for (auto& internal_toon_path : loader.internal_toon())
        {
            m_internal_toon.push_back(graphics.make_texture("resource/mmd/" + internal_toon_path));
        }

        visual& v = world.component<visual>(m_actor);
        v.vertex_buffer = module<ash::graphics::graphics>().make_vertex_buffer<vertex>(
            vertices.data(),
            vertices.size());
        v.index_buffer = module<ash::graphics::graphics>().make_index_buffer<std::int32_t>(
            indices.data(),
            indices.size());
        for (auto [index_start, index_end] : loader.submesh())
            v.submesh.push_back({index_start, index_end});

        v.material.resize(loader.materials().size());
        m_mmd_pipeline = graphics.make_render_pipeline("mmd");
        for (std::size_t i = 0; i < v.material.size(); ++i)
        {
            auto& material = loader.materials()[i];

            v.material[i].pipeline = m_mmd_pipeline.get();
            v.material[i].property = graphics.make_render_parameter("mmd_material");
            v.material[i].property->set(0, material.diffuse);
            v.material[i].property->set(1, material.specular);
            v.material[i].property->set(2, material.specular_strength);
            v.material[i].property->set(
                3,
                material.toon_index == -1 ? std::uint32_t(0) : std::uint32_t(1));
            v.material[i].property->set(4, static_cast<std::uint32_t>(material.sphere_mode));

            v.material[i].property->set(5, m_textures[material.texture_index].get());

            if (material.toon_index != -1)
            {
                if (material.toon_mode == toon_mode::TEXTURE)
                    v.material[i].property->set(6, m_textures[material.toon_index].get());
                else if (material.toon_mode == toon_mode::INTERNAL)
                    v.material[i].property->set(6, m_internal_toon[material.toon_index].get());
            }
            if (material.sphere_mode != sphere_mode::DISABLED)
                v.material[i].property->set(7, m_textures[material.sphere_index].get());
        }
        v.object = graphics.make_render_parameter("ash_object");

        transform& actor_transform = world.component<transform>(m_actor);
        actor_transform.node()->parent(module<ash::scene::scene>().root_node());

        auto& bones = loader.bones();
        std::vector<scene_node*> bone_nodes(bones.size());
        for (std::size_t i = 0; i < bones.size(); ++i)
        {
            entity_id e = world.create();
            m_skeleton.push_back(e);

            world.add<transform>(e);
            transform& t = world.component<transform>(e);

            bone_nodes[i] = t.node();

            if (bones[i].parent_index != -1)
            {
                math::float3 local_position = math::vector_plain::sub(
                    bones[i].position,
                    bones[bones[i].parent_index].position);
                t.position(local_position);
                t.node()->parent(bone_nodes[bones[i].parent_index]);
            }
            else
            {
                t.position(bones[i].position);
                t.node()->parent(actor_transform.node());
            }
        }

        std::vector<std::vector<const pmx_rigidbody*>> bone_rigidbody(bones.size());
        for (auto& rigidbody : loader.rigidbodys())
            bone_rigidbody[rigidbody.bone_index].push_back(&rigidbody);

        for (std::size_t i = 0; i < bone_rigidbody.size(); ++i)
        {
            if (bone_rigidbody[i].empty())
                continue;

            std::vector<collision_shape_interface*> shapes;
            std::vector<math::float4x4> offset(bone_rigidbody[i].size());
            for (std::size_t j = 0; j < bone_rigidbody[i].size(); ++j)
            {
                auto pmx_r = bone_rigidbody[i][j];

                collision_shape_desc desc = {};
                switch (pmx_r->shape)
                {
                case pmx_rigidbody_shape_type::SPHERE:
                    desc.type = collision_shape_type::SPHERE;
                    desc.sphere.radius = pmx_r->size[0];
                    break;
                case pmx_rigidbody_shape_type::BOX:
                    desc.type = collision_shape_type::BOX;
                    desc.box.length = pmx_r->size[0];
                    desc.box.height = pmx_r->size[1];
                    desc.box.width = pmx_r->size[2];
                    break;
                case pmx_rigidbody_shape_type::CAPSULE:
                    desc.type = collision_shape_type::CAPSULE;
                    desc.capsule.radius = pmx_r->size[0];
                    desc.capsule.height = pmx_r->size[1];
                    break;
                default:
                    break;
                }

                auto shape = module<ash::physics::physics>().make_shape(desc);
                shapes.push_back(shape.get());
                m_shapes.push_back(std::move(shape));

                math::float4_simd position_offset = math::simd::load(pmx_r->translate);
                math::float4_simd rotation_offset =
                    math::simd::load(math::quaternion_plain::rotation_euler(
                        pmx_r->rotate[1],
                        pmx_r->rotate[0],
                        pmx_r->rotate[2]));
                math::simd::store(
                    math::matrix_simd::affine_transform(
                        math::simd::set(1.0f, 1.0f, 1.0f, 0.0f),
                        rotation_offset,
                        position_offset),
                    offset[j]);
            }

            world.add<rigidbody>(m_skeleton[i]);
            rigidbody& r = world.component<rigidbody>(m_skeleton[i]);
            if (shapes.size() == 1)
            {
                r.shape(shapes[0]);
                r.offset(offset[0]);
            }
            else
            {
                auto shape = module<ash::physics::physics>().make_shape(
                    shapes.data(),
                    offset.data(),
                    shapes.size());
                r.shape(shape.get());
                m_shapes.push_back(std::move(shape));
            }
        }
    }

    void initialize_camera()
    {
        ash::ecs::world& world = module<ash::ecs::world>();

        m_camera = world.create();
        world.add<main_camera, camera, transform>(m_camera);
        camera& c_camera = world.component<camera>(m_camera);
        c_camera.set(math::to_radians(30.0f), 1300.0f / 800.0f, 0.01f, 1000.0f);

        transform& c_transform = world.component<transform>(m_camera);
        c_transform.position(0.0f, 16.0f, -38.0f);
        c_transform.rotation(0.0f, 0.0f, 0.0f, 1.0f);
        c_transform.scaling(1.0f, 1.0f, 1.0f);
        c_transform.node()->parent(module<ash::scene::scene>().root_node());
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

            visual& v = world.component<visual>(m_actor);
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

    void update_actor(float delta)
    {
        auto& world = module<ash::ecs::world>();
        auto& keyboard = module<ash::window::window>().keyboard();

        float move = 0.0f;
        if (keyboard.key(keyboard_key::KEY_E).down())
            move += 1.0f;
        if (keyboard.key(keyboard_key::KEY_Q).down())
            move -= 1.0f;

        transform& actor_transform = world.component<transform>(m_actor);
        math::float3 new_position = actor_transform.position();
        new_position[0] += move * m_move_speed * delta;
        actor_transform.position(new_position);
    }

    void update()
    {
        if (module<ash::window::window>().keyboard().key(keyboard_key::KEY_ESC).down())
            m_app->exit();

        float delta = module<ash::core::timer>().frame_delta();
        update_camera(delta);
        update_actor(delta);
    }

    std::string m_title;
    application* m_app;

    entity_id m_camera;
    entity_id m_actor;

    std::vector<entity_id> m_skeleton;

    std::vector<std::unique_ptr<resource>> m_textures;
    std::vector<std::unique_ptr<resource>> m_internal_toon;

    std::unique_ptr<render_pipeline> m_mmd_pipeline;

    std::vector<std::unique_ptr<collision_shape_interface>> m_shapes;

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
    app.install<ash::sample::mmd::test_module>(&app);

    app.run();

    return 0;
}