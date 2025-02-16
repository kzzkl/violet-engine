#include "components/camera_component.hpp"
#include "components/collider_component.hpp"
#include "components/joint_component.hpp"
#include "components/mesh_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/rigidbody_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "ecs_command/ecs_command_system.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/renderers/deferred_renderer.hpp"
#include "physics/physics_system.hpp"
#include "scene/hierarchy_system.hpp"
#include "scene/scene_system.hpp"
#include "scene/transform_system.hpp"
#include "task/task_graph_printer.hpp"
#include "window/window_system.hpp"

namespace violet
{
class physics_demo : public system
{
public:
    physics_demo()
        : system("physics_demo")
    {
    }

    void install(application& app) override
    {
        m_app = &app;
    }

    bool initialize(const dictionary& config) override
    {
        auto& window = get_system<window_system>();
        window.on_resize().add_task().set_execute(
            [this]()
            {
                resize();
            });
        window.on_destroy().add_task().set_execute(
            [this]()
            {
                m_app->exit();
            });

        task_graph& task_graph = get_task_graph();
        task_group& update = task_graph.get_group("Update");

        task_graph.add_task()
            .set_name("Demo Tick")
            .set_group(update)
            .set_execute(
                [this]()
                {
                    tick();
                });

        task_graph.reset();
        task_graph_printer::print(task_graph);

        initialize_render();
        initialize_scene();

        resize();

        return true;
    }

private:
    void initialize_render()
    {
        m_swapchain = render_device::instance().create_swapchain({
            .flags = RHI_TEXTURE_TRANSFER_DST | RHI_TEXTURE_RENDER_TARGET,
            .window_handle = get_system<window_system>().get_handle(),
        });
        m_renderer = std::make_unique<deferred_renderer>();

        m_geometry = std::make_unique<box_geometry>();
        m_material = std::make_unique<unlit_material>();
    }

    void initialize_scene()
    {
        auto& world = get_world();

        m_cube1 = world.create();
        world.add_component<
            transform_component,
            mesh_component,
            rigidbody_component,
            collider_component,
            scene_component>(m_cube1);

        auto& cube1_mesh = world.get_component<mesh_component>(m_cube1);
        cube1_mesh.geometry = m_geometry.get();
        cube1_mesh.submeshes.push_back({
            .vertex_offset = 0,
            .index_offset = 0,
            .index_count = m_geometry->get_index_count(),
            .material = m_material.get(),
        });

        auto& cube1_transform = world.get_component<transform_component>(m_cube1);
        cube1_transform.set_position({0.0f, 0.0f, 0.0f});

        auto& cube1_rigidbody = world.get_component<rigidbody_component>(m_cube1);
        cube1_rigidbody.type = PHY_RIGIDBODY_TYPE_STATIC;

        auto& cube1_collider = world.get_component<collider_component>(m_cube1);
        cube1_collider.shapes.push_back(collider_shape{
            .shape =
                {
                    .type = PHY_COLLISION_SHAPE_TYPE_BOX,
                    .box =
                        {
                            .width = 1.0f,
                            .height = 1.0f,
                            .length = 1.0f,
                        },
                },
        });

        m_cube2 = world.create();
        world.add_component<
            transform_component,
            mesh_component,
            rigidbody_component,
            collider_component,
            joint_component,
            scene_component>(m_cube2);

        auto& cube2_mesh = world.get_component<mesh_component>(m_cube2);
        cube2_mesh.geometry = m_geometry.get();
        cube2_mesh.submeshes.push_back({
            .vertex_offset = 0,
            .index_offset = 0,
            .index_count = m_geometry->get_index_count(),
            .material = m_material.get(),
        });

        auto& cube2_transform = world.get_component<transform_component>(m_cube2);
        cube2_transform.set_position({2.0f, 5.0f, 0.0f});

        auto& cube2_rigidbody = world.get_component<rigidbody_component>(m_cube2);
        cube2_rigidbody.type = PHY_RIGIDBODY_TYPE_DYNAMIC;
        cube2_rigidbody.mass = 1.0f;

        auto& cube2_collider = world.get_component<collider_component>(m_cube2);
        cube2_collider.shapes.push_back(collider_shape{
            .shape =
                {
                    .type = PHY_COLLISION_SHAPE_TYPE_BOX,
                    .box =
                        {
                            .width = 1.0f,
                            .height = 1.0f,
                            .length = 1.0f,
                        },
                },
        });

        auto& cube2_joint = world.get_component<joint_component>(m_cube2);
        cube2_joint.joints.emplace_back(joint{
            .target = m_cube1,
            .min_linear = {-5.0f, -5.0f, -5.0f},
            .max_linear = {5.0f, 5.0f, 5.0f},
        });
        cube2_joint.joints[0].spring_enable[0] = true;
        cube2_joint.joints[0].stiffness[0] = 100.0f;
        cube2_joint.joints[0].damping[0] = 5.0f;

        m_camera = world.create();
        world.add_component<
            transform_component,
            camera_component,
            orbit_control_component,
            scene_component>(m_camera);

        auto& camera_transform = world.get_component<transform_component>(m_camera);
        camera_transform.set_position({0.0f, 0.0f, -10.0f});

        auto& main_camera = world.get_component<camera_component>(m_camera);
        main_camera.renderer = m_renderer.get();
        main_camera.render_target = m_swapchain.get();
    }

    void tick()
    {
        auto& window = get_system<window_system>();

        if (window.get_keyboard().key(KEYBOARD_KEY_SPACE).press())
        {
            auto& world = get_world();

            auto* command = get_system<ecs_command_system>().allocate_command();
            command->destroy(m_cube2);
        }
    }

    void resize()
    {
        m_swapchain->resize();
    }

    entity m_camera;
    entity m_cube1;
    entity m_cube2;
    entity m_plane;

    std::unique_ptr<geometry> m_geometry;
    std::unique_ptr<material> m_material;

    rhi_ptr<rhi_swapchain> m_swapchain;
    std::unique_ptr<renderer> m_renderer;

    application* m_app{nullptr};
};
} // namespace violet

int main()
{
    violet::application app("assets/config/physics.json");
    app.install<violet::ecs_command_system>();
    app.install<violet::hierarchy_system>();
    app.install<violet::transform_system>();
    app.install<violet::scene_system>();
    app.install<violet::window_system>();
    app.install<violet::graphics_system>();
    app.install<violet::physics_system>();
    app.install<violet::control_system>();
    app.install<violet::physics_demo>();

    app.run();

    return 0;
}