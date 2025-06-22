#include "cluster_material.hpp"
#include "components/camera_component.hpp"
#include "components/hierarchy_component.hpp"
#include "components/mesh_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "gltf_loader.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/physical_material.hpp"
#include "graphics/renderers/deferred_renderer.hpp"
#include "graphics/tools/geometry_tool.hpp"
#include "window/window_system.hpp"

namespace violet
{
class cluster_demo : public system
{
public:
    cluster_demo()
        : system("Cluster Demo")
    {
    }

    void install(application& app) override
    {
        app.install<window_system>();
        app.install<graphics_system>();
        app.install<control_system>();

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

        m_camera = world.create();
        world.add_component<
            transform_component,
            camera_component,
            orbit_control_component,
            scene_component>(m_camera);

        auto& camera_control = world.get_component<orbit_control_component>(m_camera);
        camera_control.radius_speed = 0.2f;
        camera_control.target = {0.0f, 0.1f, 0.0f};

        auto& camera_transform = world.get_component<transform_component>(m_camera);
        camera_transform.set_position({0.0f, 0.0f, -10.0f});

        auto& main_camera = world.get_component<camera_component>(m_camera);
        main_camera.renderer = std::make_unique<deferred_renderer>();
        main_camera.render_target = m_swapchain.get();

        // Model.
        m_material = std::make_unique<cluster_material>();

        if (!model_path.empty())
        {
            load_model(model_path);
        }
    }

    void resize()
    {
        m_swapchain->resize();
    }

    void tick() {}

    void load_model(std::string_view path)
    {
        auto& world = get_world();

        gltf_loader loader;

        auto result = loader.load(path);
        if (!result)
        {
            return;
        }

        model model = {};

        for (const auto& geometry_data : result->geometries)
        {
            auto model_geometry = std::make_unique<geometry>();
            model_geometry->set_positions(geometry_data.positions);
            model_geometry->set_normals(geometry_data.normals);
            model_geometry->set_tangents(geometry_data.tangents);
            model_geometry->set_texcoords(geometry_data.texcoords);
            model_geometry->set_indexes(geometry_data.indexes);

            geometry_tool::cluster_input input = {
                .positions = geometry_data.positions,
                .indexes = geometry_data.indexes,
            };

            for (const auto& submesh_data : geometry_data.submeshes)
            {
                input.submeshes.push_back({
                    .vertex_offset = submesh_data.vertex_offset,
                    .index_offset = submesh_data.index_offset,
                    .index_count = submesh_data.index_count,
                });
            }

            // auto output = geometry_tool::generate_clusters(input);
            // output.save("stanford-bunny.cluster");

            geometry_tool::cluster_output output;
            output.load("stanford-bunny.cluster");

            model_geometry->set_positions(output.positions);
            model_geometry->set_indexes(output.indexes);

            for (const auto& submesh : output.submeshes)
            {
                model_geometry->add_submesh(submesh.clusters, submesh.cluster_nodes);
            }

            model.geometries.push_back(std::move(model_geometry));
        }

        for (const auto& material_data : result->materials)
        {
            auto model_material = std::make_unique<physical_material>();
            model_material->set_albedo(material_data.albedo);
            model_material->set_roughness(material_data.roughness);
            model_material->set_metallic(material_data.metallic);
            model_material->set_emissive(material_data.emissive);

            if (material_data.albedo_texture != nullptr)
            {
                model_material->set_albedo(material_data.albedo_texture);
            }

            if (material_data.roughness_metallic_texture != nullptr)
            {
                model_material->set_roughness_metallic(material_data.roughness_metallic_texture);
            }

            if (material_data.emissive_texture != nullptr)
            {
                model_material->set_emissive(material_data.emissive_texture);
            }

            if (material_data.normal_texture != nullptr)
            {
                model_material->set_normal(material_data.normal_texture);
            }

            model.materials.push_back(std::move(model_material));
        }

        for (const auto& node_data : result->nodes)
        {
            entity entity = world.create();
            world.add_component<transform_component, scene_component>(entity);

            auto& transform = world.get_component<transform_component>(entity);
            transform.set_position(node_data.position);
            transform.set_rotation(node_data.rotation);
            transform.set_scale(node_data.scale);

            model.entities.push_back(entity);
        }

        for (std::size_t i = 0; i < result->nodes.size(); ++i)
        {
            auto& node = result->nodes[i];
            auto entity = model.entities[i];

            if (node.mesh != -1)
            {
                world.add_component<mesh_component>(entity);

                auto& mesh_data = result->meshes[node.mesh];

                auto& entity_mesh = world.get_component<mesh_component>(entity);
                entity_mesh.geometry = model.geometries[mesh_data.geometry].get();

                for (auto& submesh : mesh_data.submeshes)
                {
                    entity_mesh.submeshes.push_back({
                        .index = submesh,
                        .material = m_material.get(),
                    });
                }

                m_mesh = entity;
            }

            if (node.parent != -1)
            {
                world.add_component<parent_component>(entity);
                world.get_component<parent_component>(entity).parent = model.entities[node.parent];
            }
        }

        m_models.push_back(std::move(model));

        // int size = 50;
        // for (int i = 0; i < size; ++i)
        // {
        //     for (int j = 0; j < size; ++j)
        //     {
        //         for (int k = 0; k < size; ++k)
        //         {
        //             entity entity = world.create();
        //             world.add_component<
        //                 transform_component,
        //                 scene_component,
        //                 mesh_component,
        //                 parent_component>(entity);

        //             auto& transform = world.get_component<transform_component>(entity);
        //             transform.set_position(
        //                 {static_cast<float>(i - (size >> 1)) * 0.2f,
        //                  static_cast<float>(j - (size >> 1)) * 0.2f,
        //                  static_cast<float>(k - (size >> 1)) * 0.2f});

        //             auto& parent = world.get_component<parent_component>(entity);
        //             parent.parent = m_mesh;

        //             auto& mesh = world.get_component<mesh_component>(entity);
        //             mesh.geometry = m_models[0].geometries[0].get();
        //             mesh.submeshes.push_back({
        //                 .index = 0,
        //                 .material = m_material.get(),
        //             });
        //         }
        //     }
        // }
    }

    entity m_camera;
    entity m_mesh;

    struct model
    {
        std::vector<std::unique_ptr<geometry>> geometries;
        std::vector<std::unique_ptr<material>> materials;
        std::vector<entity> entities;
    };

    std::vector<model> m_models;
    std::unique_ptr<material> m_material;

    rhi_ptr<rhi_swapchain> m_swapchain;

    application* m_app{nullptr};
};
} // namespace violet

int main()
{
    violet::application app("assets/config/cluster.json");
    app.install<violet::cluster_demo>();
    app.run();

    return 0;
}