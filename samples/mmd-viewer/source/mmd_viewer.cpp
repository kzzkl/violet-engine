#include "mmd_viewer.hpp"
#include "components/camera_component.hpp"
#include "components/mesh_component.hpp"
// #include "components/mmd_skeleton_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/rigidbody_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/renderers/deferred_renderer.hpp"
#include "physics/physics_system.hpp"
#include "window/window_system.hpp"

namespace violet::sample
{
mmd_viewer::mmd_viewer()
    : engine_system("mmd viewer")
{
}

mmd_viewer::~mmd_viewer() {}

bool mmd_viewer::initialize(const dictionary& config)
{
    m_pmx_path = config["pmx"];
    m_vmd_path = config["vmd"];

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

    initialize_render();
    initialize_scene();

    resize();

    return true;
}

void mmd_viewer::initialize_render()
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

    m_material = std::make_unique<unlit_material>();
}

void mmd_viewer::initialize_scene()
{
    auto& world = get_world();

    m_camera = world.create();
    world.add_component<
        transform_component,
        camera_component,
        orbit_control_component,
        scene_component>(m_camera);

    auto& camera_transform = world.get_component<transform_component>(m_camera);
    camera_transform.position = {0.0f, 0.0f, -10.0f};

    auto& main_camera = world.get_component<camera_component>(m_camera);
    main_camera.renderer = m_renderer.get();
    main_camera.render_targets = {m_swapchain.get()};

    mmd_loader loader(m_pmx_path, m_vmd_path);
    if (auto result = loader.load())
    {
        m_model_data = std::move(*result);

        std::vector<entity> entities;
        for (auto& node : m_model_data.nodes)
        {
            entity entity = world.create();
            world.add_component<transform_component, mesh_component, scene_component>(entity);

            auto& mesh_data = m_model_data.meshes[node.mesh];

            auto& entity_mesh = world.get_component<mesh_component>(entity);
            entity_mesh.geometry = m_model_data.geometries[mesh_data.geometry].get();
            for (auto& submesh_data : mesh_data.submeshes)
            {
                entity_mesh.submeshes.push_back({
                    .vertex_offset = submesh_data.vertex_offset,
                    .index_offset = submesh_data.index_offset,
                    .index_count = submesh_data.index_count,
                    .material = m_material.get(),
                });
            }

            auto& entity_transform = world.get_component<transform_component>(entity);
            entity_transform.position = node.position;
            entity_transform.rotation = node.rotation;
            entity_transform.scale = node.scale;
        }
    }
}

void mmd_viewer::tick(float delta) {}

void mmd_viewer::resize()
{
    auto extent = get_system<window_system>().get_extent();

    m_swapchain->resize(extent.width, extent.height);

    auto& main_camera = get_world().get_component<camera_component>(m_camera);
    main_camera.render_targets[0] = m_swapchain.get();
}
} // namespace violet::sample