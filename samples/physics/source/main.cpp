#include "components/camera_component.hpp"
#include "components/collider_component.hpp"
#include "components/joint_component.hpp"
#include "components/mesh_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/rigidbody_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "core/engine.hpp"
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

namespace violet::sample
{
/*class color_pipeline : public render_pipeline
{
public:
    color_pipeline(std::string_view name, renderer* context) : render_pipeline(name, context)
    {
        set_shader("physics/shaders/basic.vs", "physics/shaders/basic.fs");
        set_vertex_attributes({
            {"position", RHI_RESOURCE_FORMAT_R32G32B32_FLOAT},
            {"color",    RHI_RESOURCE_FORMAT_R32G32B32_FLOAT}
        });
        set_cull_mode(RHI_CULL_MODE_NONE);

        rhi_parameter_layout* material_layout = context->add_parameter_layout(
            "color pipeline",
            {
                {RHI_PARAMETER_TYPE_TEXTURE, 1}
        });

        set_parameter_layouts({
            {context->get_parameter_layout("violet mesh"),   RENDER_PIPELINE_PARAMETER_TYPE_MESH    },
            {material_layout,                                RENDER_PIPELINE_PARAMETER_TYPE_MATERIAL},
            {context->get_parameter_layout("violet camera"),
             RENDER_PIPELINE_PARAMETER_TYPE_CAMERA                                                  }
        });
    }

private:
    void render(rhi_render_command* command, render_data& data)
    {
        command->set_render_parameter(2, data.camera);
        for (render_mesh& mesh : data.meshes)
        {
            command->set_vertex_buffers(mesh.vertex_buffers.data(), mesh.vertex_buffers.size());
            command->set_index_buffer(mesh.index_buffer);
            command->set_render_parameter(0, mesh.transform);
            command->set_render_parameter(1, mesh.material);
            command->draw_indexed(0, 36, 0);
        }
    }
};

class physics_debug : public pei_debug_draw
{
public:
    physics_debug(
        render_graph* render_graph,
        render_subpass* debug_subpass,
        renderer* renderer,
        world& world)
        : m_position(2048),
          m_color(2048)
    {
        render_pipeline* pipeline = debug_subpass->add_pipeline<debug_pipeline>("debug");
        material_layout* layout = render_graph->add_material_layout("debug");
        layout->add_pipeline(pipeline);

        material* material = layout->add_material("debug");
        m_geometry = std::make_unique<geometry>(renderer);

        m_geometry->add_attribute(
            "position",
            m_position,
            RHI_BUFFER_FLAG_VERTEX | RHI_BUFFER_FLAG_HOST_VISIBLE);
        m_geometry->add_attribute(
            "color",
            m_color,
            RHI_BUFFER_FLAG_VERTEX | RHI_BUFFER_FLAG_HOST_VISIBLE);
        m_position.clear();
        m_color.clear();

        m_object = std::make_unique<actor>("physics debug", world);
        auto [transform_ptr, mesh_ptr] = m_object->add<transform, mesh>();

        mesh_ptr->set_geometry(m_geometry.get());
        mesh_ptr->add_submesh(0, 0, 0, 0, material);
    }

    void tick()
    {
        component_ptr<mesh> mesh_ptr = m_object->get<mesh>();
        std::memcpy(
            mesh_ptr->get_geometry()->get_vertex_buffer("position")->get_buffer(),
            m_position.data(),
            m_position.size() * sizeof(float3));
        std::memcpy(
            mesh_ptr->get_geometry()->get_vertex_buffer("color")->get_buffer(),
            m_color.data(),
            m_color.size() * sizeof(float3));
        mesh_ptr->set_submesh(0, 0, m_position.size(), 0, 0);

        m_position.clear();
        m_color.clear();
    }

    virtual void draw_line(const float3& start, const float3& end, const float3& color) override
    {
        m_position.push_back(start);
        m_position.push_back(end);
        m_color.push_back(color);
        m_color.push_back(color);
    }

private:
    std::unique_ptr<geometry> m_geometry;

    std::unique_ptr<actor> m_object;

    std::vector<float3> m_position;
    std::vector<float3> m_color;
};*/

class physics_demo : public engine_system
{
public:
    physics_demo()
        : engine_system("physics_demo")
    {
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
            []()
            {
                // engine::exit();
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
        auto window_extent = get_system<window_system>().get_extent();

        m_swapchain = render_device::instance().create_swapchain({
            .extent =
                {
                    .width = window_extent.width,
                    .height = window_extent.height,
                },
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
        main_camera.render_targets = {m_swapchain.get()};
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
        auto extent = get_system<window_system>().get_extent();

        m_swapchain->resize(extent.width, extent.height);

        auto& main_camera = get_world().get_component<camera_component>(m_camera);
        main_camera.render_targets[0] = m_swapchain.get();
    }

    entity m_camera;
    entity m_cube1;
    entity m_cube2;
    entity m_plane;

    std::unique_ptr<geometry> m_geometry;
    std::unique_ptr<material> m_material;

    rhi_ptr<rhi_swapchain> m_swapchain;
    std::unique_ptr<renderer> m_renderer;
};
} // namespace violet::sample

int main()
{
    violet::engine::initialize("physics/config");
    violet::engine::install<violet::ecs_command_system>();
    violet::engine::install<violet::hierarchy_system>();
    violet::engine::install<violet::transform_system>();
    violet::engine::install<violet::scene_system>();
    violet::engine::install<violet::window_system>();
    violet::engine::install<violet::graphics_system>();
    violet::engine::install<violet::physics_system>();
    violet::engine::install<violet::control_system>();
    violet::engine::install<violet::sample::physics_demo>();

    violet::engine::run();

    return 0;
}