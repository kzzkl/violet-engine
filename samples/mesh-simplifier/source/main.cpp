#include "components/camera_component.hpp"
#include "components/light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "deferred_renderer_imgui.hpp"
#include "gltf_loader.hpp"
#include "graphics/geometries/sphere_geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/renderers/features/taa_render_feature.hpp"
#include "graphics/tools/geometry_tool.hpp"
#include "imgui.h"
#include "imgui_system.hpp"
#include "window/window_system.hpp"
#include "graphics/materials/physical_material.hpp"

namespace violet
{
class mesh_simplifier_demo : public system
{
public:
    mesh_simplifier_demo()
        : system("mesh_simplifier_demo")
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
        main_camera.renderer->get_feature<taa_render_feature>()->disable();
        main_camera.render_target = m_swapchain.get();

        // Model.
        // m_material = std::make_unique<unlit_material>(
        //     RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        //     RHI_CULL_MODE_NONE,
        //     RHI_POLYGON_MODE_LINE);
        m_material = std::make_unique<physical_material>();

        if (!model_path.empty())
        {
            gltf_loader loader;
            if (auto result = loader.load(model_path))
            {
                const auto& geometry_data = result->geometries[0];

                m_original_geometry = std::make_unique<geometry>();
                m_original_geometry->set_positions(geometry_data.positions);
                m_original_geometry->set_normals(geometry_data.normals);
                m_original_geometry->set_tangents(geometry_data.tangents);
                m_original_geometry->set_texcoords(geometry_data.texcoords);
                m_original_geometry->set_indexes(geometry_data.indexes);
                for (const auto& submesh : geometry_data.submeshes)
                {
                    m_original_geometry->add_submesh(
                        submesh.vertex_offset,
                        submesh.index_offset,
                        submesh.index_count);
                }
            }
            else
            {
                m_original_geometry = std::make_unique<sphere_geometry>(0.5f);
            }
        }
        else
        {
            m_original_geometry = std::make_unique<sphere_geometry>(0.5f);
        }

        m_model = world.create();
        world.add_component<transform_component, mesh_component, scene_component>(m_model);

        auto& mesh = world.get_component<mesh_component>(m_model);
        mesh.geometry = m_original_geometry.get();
        mesh.submeshes.push_back({
            .index = 0,
            .material = m_material.get(),
        });

        m_simplified_geometry = std::make_unique<geometry>();
        m_simplified_geometry->set_positions(m_original_geometry->get_positions());
        m_simplified_geometry->set_normals(m_original_geometry->get_normals());
        m_simplified_geometry->set_indexes(m_original_geometry->get_indexes());
        m_simplified_geometry->add_submesh(0, 0, m_original_geometry->get_indexes().size());
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
            mesh.geometry =
                show_simplified ? m_simplified_geometry.get() : m_original_geometry.get();
        }

        static float simplify_ratio = 1.0f;
        if (ImGui::SliderFloat("Simplify Ratio", &simplify_ratio, 0.0f, 1.0f))
        {
            std::uint32_t triangle_count = m_original_geometry->get_index_count() / 3;
            auto target_triangle_count =
                static_cast<std::uint32_t>(static_cast<float>(triangle_count) * simplify_ratio);

            auto output = geometry_tool::simplify({
                .positions = m_original_geometry->get_positions(),
                .normals = m_original_geometry->get_normals(),
                .indexes = m_original_geometry->get_indexes(),
                .target_triangle_count = target_triangle_count,
            });

            m_simplified_geometry->set_positions(output.positions);
            m_simplified_geometry->set_normals(output.normals);
            m_simplified_geometry->set_indexes(output.indexes);
            m_simplified_geometry->clear_submeshes();
            m_simplified_geometry->add_submesh(0, 0, output.indexes.size());
        }
    }

    entity m_light;
    entity m_camera;
    entity m_model;

    std::unique_ptr<material> m_material;
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