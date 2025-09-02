#include "sample/sample_system.hpp"
#include "components/camera_component.hpp"
#include "components/hierarchy_component.hpp"
#include "components/light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/scene_component.hpp"
#include "components/skybox_component.hpp"
#include "control/control_system.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/pbr_material.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "sample/deferred_renderer_imgui.hpp"
#include "sample/gltf_loader.hpp"
#include "sample/imgui_system.hpp"
#include "window/window_system.hpp"

namespace violet
{
sample_system::sample_system(std::string_view name)
    : system(name)
{
}

void sample_system::install(application& app)
{
    app.install<window_system>();
    app.install<graphics_system>();
    app.install<control_system>();
    app.install<imgui_system>();

    m_app = &app;
}

bool sample_system::initialize(const dictionary& config)
{
    auto& window = get_system<window_system>();
    window.on_resize().add_task().set_execute(
        [this]()
        {
            m_swapchain->resize();
        });
    window.on_destroy().add_task().set_execute(
        [this]()
        {
            m_app->exit();
        });

    task_graph& task_graph = get_task_graph();
    task_group& update = task_graph.get_group("Update");

    task_graph.add_task()
        .set_name(get_name())
        .set_group(update)
        .set_options(TASK_OPTION_MAIN_THREAD)
        .set_execute(
            [this]()
            {
                tick();
            });

    initialize_render();
    initialize_scene(config["skybox"]);

    m_swapchain->resize();

    return true;
}

entity sample_system::load_model(std::string_view model_path)
{
    auto& world = get_world();

    gltf_loader loader;
    auto result = loader.load(model_path);
    if (!result)
    {
        return INVALID_ENTITY;
    }

    for (auto& texture : result->textures)
    {
        m_textures.push_back(std::move(texture));
    }

    std::size_t geometry_offset = m_geometries.size();
    for (const auto& geometry_data : result->geometries)
    {
        auto model_geometry = std::make_unique<geometry>();
        model_geometry->set_positions(geometry_data.positions);
        model_geometry->set_normals(geometry_data.normals);
        model_geometry->set_tangents(geometry_data.tangents);
        model_geometry->set_texcoords(geometry_data.texcoords);
        model_geometry->set_indexes(geometry_data.indexes);

        for (const auto& submesh : geometry_data.submeshes)
        {
            model_geometry->add_submesh(
                submesh.vertex_offset,
                submesh.index_offset,
                submesh.index_count);
        }

        m_geometries.push_back(std::move(model_geometry));
    }

    std::size_t material_offset = m_materials.size();
    for (const auto& material_data : result->materials)
    {
        auto model_material = std::make_unique<pbr_material>();
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

        m_materials.push_back(std::move(model_material));
    }

    entity root = world.create();
    world.add_component<transform_component, scene_component>(root);

    std::vector<entity> entities;

    for (const auto& node_data : result->nodes)
    {
        entity entity = world.create();
        world.add_component<transform_component, parent_component, scene_component>(entity);

        auto& transform = world.get_component<transform_component>(entity);
        transform.set_position(node_data.position);
        transform.set_rotation(node_data.rotation);
        transform.set_scale(node_data.scale);

        entities.push_back(entity);
    }

    for (std::size_t i = 0; i < result->nodes.size(); ++i)
    {
        const auto& node_data = result->nodes[i];
        auto entity = entities[i];

        if (node_data.mesh != -1)
        {
            world.add_component<mesh_component>(entity);

            const auto& mesh_data = result->meshes[node_data.mesh];

            auto& entity_mesh = world.get_component<mesh_component>(entity);
            entity_mesh.geometry = m_geometries[mesh_data.geometry + geometry_offset].get();

            for (std::size_t j = 0; j < mesh_data.submeshes.size(); ++j)
            {
                std::size_t material_index =
                    mesh_data.materials[j] == -1 ? 0 : mesh_data.materials[j] + material_offset;

                entity_mesh.submeshes.push_back({
                    .index = mesh_data.submeshes[j],
                    .material = m_materials[material_index].get(),
                });
            }
        }

        if (node_data.parent != -1)
        {
            world.get_component<parent_component>(entity).parent = entities[node_data.parent];
        }
        else
        {
            world.get_component<parent_component>(entity).parent = root;
        }
    }

    return root;
}

void sample_system::initialize_render()
{
    m_swapchain = render_device::instance().create_swapchain({
        .flags = RHI_TEXTURE_TRANSFER_DST | RHI_TEXTURE_RENDER_TARGET,
        .window_handle = get_system<window_system>().get_handle(),
    });

    m_materials.push_back(std::make_unique<unlit_material>());
}

void sample_system::initialize_scene(std::string_view skybox_path)
{
    auto& world = get_world();

    m_skybox = std::make_unique<skybox>(skybox_path);

    entity scene_skybox = world.create();
    world.add_component<transform_component, skybox_component, scene_component>(scene_skybox);
    auto& skybox = world.get_component<skybox_component>(scene_skybox);
    skybox.skybox = m_skybox.get();

    m_light = world.create();
    world.add_component<transform_component, light_component, scene_component>(m_light);

    auto& light_transform = world.get_component<transform_component>(m_light);
    light_transform.set_position({10.0f, 10.0f, 10.0f});
    light_transform.lookat({0.0f, 0.0f, 0.0f});

    auto& main_light = world.get_component<light_component>(m_light);
    main_light.type = LIGHT_DIRECTIONAL;
    main_light.color = {.x = 1.0f, .y = 1.0f, .z = 1.0f};

    m_camera = world.create();
    world.add_component<
        transform_component,
        camera_component,
        orbit_control_component,
        scene_component>(m_camera);

    auto& camera_transform = world.get_component<transform_component>(m_camera);
    camera_transform.set_position({0.0f, 0.0f, -10.0f});

    auto& main_camera = world.get_component<camera_component>(m_camera);
    main_camera.renderer = std::make_unique<deferred_renderer_imgui>();
    main_camera.render_target = m_swapchain.get();
}
} // namespace violet