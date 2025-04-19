#include "components/camera_component.hpp"
#include "components/light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "deferred_renderer_imgui.hpp"
#include "gltf_loader.hpp"
#include "graphics/geometries/plane_geometry.hpp"
#include "graphics/geometries/sphere_geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/tools/geometry_tool.hpp"
#include "imgui.h"
#include "imgui_system.hpp"
#include "window/window_system.hpp"

namespace violet
{
class mesh_simplifier_demo : public system
{
public:
    mesh_simplifier_demo()
        : system("Mesh Simplifier Demo")
    {
    }

    void install(application& app) override
    {
        app.install<window_system>();
        app.install<graphics_system>();
        app.install<control_system>();
        app.install<imgui_system>();

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
            .set_options(TASK_OPTION_MAIN_THREAD)
            .set_execute(
                [this]()
                {
                    tick();
                });

        initialize_render();
        initialize_scene(config["model"]);

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
    }

    void initialize_scene(std::string_view model_path)
    {
        auto& world = get_world();

        m_light = world.create();
        world.add_component<transform_component, light_component, scene_component>(m_light);

        auto& light_transform = world.get_component<transform_component>(m_light);
        light_transform.set_position({10.0f, 10.0f, 10.0f});
        light_transform.lookat({0.0f, 0.0f, 0.0f});

        auto& main_light = world.get_component<light_component>(m_light);
        main_light.type = LIGHT_DIRECTIONAL;
        main_light.color = {1.0f, 1.0f, 1.0f};

        m_camera = world.create();
        world.add_component<
            transform_component,
            camera_component,
            orbit_control_component,
            scene_component>(m_camera);

        auto& camera_control = world.get_component<orbit_control_component>(m_camera);
        camera_control.radius_speed = 0.05f;

        auto& camera_transform = world.get_component<transform_component>(m_camera);
        camera_transform.set_position({0.0f, 0.0f, -10.0f});

        auto& main_camera = world.get_component<camera_component>(m_camera);
        main_camera.renderer = std::make_unique<deferred_renderer_imgui>();
        main_camera.render_target = m_swapchain.get();

        // Model.
        m_material = std::make_unique<unlit_material>(
            RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            RHI_CULL_MODE_NONE,
            RHI_POLYGON_MODE_LINE);

        if (model_path.empty())
        {
            m_original_geometry = std::make_unique<sphere_geometry>(0.5f);
            // m_original_geometry = std::make_unique<plane_geometry>(1.0f, 1.0f, 4, 4);
        }
        else
        {
            gltf_loader loader(model_path);
            if (auto result = loader.load())
            {
                m_original_geometry = std::move(result->geometries[0]);
            }
            else
            {
                m_original_geometry = std::make_unique<sphere_geometry>(0.5f);
            }
        }

        m_model = world.create();
        world.add_component<transform_component, mesh_component, scene_component>(m_model);

        auto& mesh = world.get_component<mesh_component>(m_model);
        mesh.geometry = m_original_geometry.get();
        mesh.submeshes.push_back({
            .vertex_offset = 0,
            .index_offset = 0,
            .index_count = m_original_geometry->get_index_count(),
            .material = m_material.get(),
        });

        m_simplified_geometry = std::make_unique<geometry>();
        m_simplified_geometry->set_position(m_original_geometry->get_position());
        m_simplified_geometry->set_indexes(m_original_geometry->get_indexes());
    }

    void resize()
    {
        m_swapchain->resize();
    }

    void tick()
    {
        auto& world = get_world();

        static bool show_simplified = false;
        if (ImGui::Checkbox("Simplified", &show_simplified))
        {
            auto& mesh = world.get_component<mesh_component>(m_model);
            if (show_simplified)
            {
                mesh.geometry = m_simplified_geometry.get();
                mesh.submeshes[0].index_count = m_simplified_geometry->get_index_count();
            }
            else
            {
                mesh.geometry = m_original_geometry.get();
                mesh.submeshes[0].index_count = m_original_geometry->get_index_count();
            }
        }

        static float simplify_ratio = 1.0f;
        if (ImGui::SliderFloat("Simplify Ratio", &simplify_ratio, 0.0f, 1.0f))
        {
            std::size_t triangle_count = m_original_geometry->get_index_count() / 3;
            auto target_triangle_count =
                static_cast<std::size_t>(static_cast<float>(triangle_count) * simplify_ratio);

            auto result = geometry_tool::simplify(
                m_original_geometry->get_position(),
                m_original_geometry->get_indexes(),
                target_triangle_count);

            m_simplified_geometry->set_position(result.positions);
            m_simplified_geometry->set_indexes(result.indexes);

            if (show_simplified)
            {
                auto& mesh = world.get_component<mesh_component>(m_model);
                mesh.submeshes[0].index_count = m_simplified_geometry->get_index_count();
            }
        }
    }

    entity m_light;
    entity m_camera;
    entity m_model;

    std::unique_ptr<unlit_material> m_material;
    std::unique_ptr<geometry> m_original_geometry;
    std::unique_ptr<geometry> m_simplified_geometry;

    rhi_ptr<rhi_swapchain> m_swapchain;

    application* m_app{nullptr};
};
} // namespace violet

int main()
{
    violet::application app("assets/config/mesh-simplifier.json");
    app.install<violet::mesh_simplifier_demo>();
    app.run();

    return 0;
}