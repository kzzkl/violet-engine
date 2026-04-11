#include "sample/sample_system.hpp"
#include "components/atmosphere_component.hpp"
#include "components/camera_component.hpp"
#include "components/first_person_control_component.hpp"
#include "components/hierarchy_component.hpp"
#include "components/light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/scene_component.hpp"
#include "components/skybox_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/pbr_material.hpp"
#include "graphics/render_graph/rdg_profiling.hpp"
#include "graphics/texture_loader.hpp"
#include "math/quaternion.hpp"
#include "sample/deferred_renderer_imgui.hpp"
#include "sample/imgui_system.hpp"
#include "sample/mesh_loader.hpp"
#include "tools/texture_tool.hpp"
#include "window/window_system.hpp"
#include <imgui.h>

namespace violet
{
namespace
{
void imgui_profiling_event(
    const std::vector<rdg_profiling::node>& nodes,
    std::uint32_t index,
    float frame_time,
    std::string_view path)
{
    const auto& node = nodes[index];

    ImGui::TableNextRow();

    ImGui::TableNextColumn();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow;

    if (node.children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    std::string name = std::format("{}##{}", node.name, path);
    bool open = ImGui::TreeNodeEx(name.c_str(), flags);

    ImGui::TableNextColumn();
    ImGui::Text("%.2f ms", node.time_ms);

    ImGui::TableNextColumn();

    float percent = frame_time > 0.0f ? node.time_ms / frame_time : 0.0f;
    ImGui::ProgressBar(percent, ImVec2(-FLT_MIN, 0), std::format("{:.2f}%", percent * 100).c_str());

    if (open)
    {
        for (std::uint32_t c : node.children)
        {
            imgui_profiling_event(nodes, c, frame_time, std::format("{}:{}", path, node.name));
        }

        ImGui::TreePop();
    }
}
} // namespace

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
    initialize_scene(config.contains("skybox") ? config["skybox"] : "");

    m_swapchain->resize();

    return true;
}

entity sample_system::load_model(std::string_view model_path, load_options options)
{
    auto& world = get_world();

    mesh_loader::scene_data scene_data;

    bool result = mesh_loader::load(
        model_path,
        scene_data,
        options & LOAD_OPTION_GENERATE_CLUSTERS,
        options & LOAD_OPTION_GENERATE_MIPMAPS,
        options & LOAD_OPTION_COMPRESS_TEXTURES);

    if (!result)
    {
        return INVALID_ENTITY;
    }

    std::size_t texture_offset = m_textures.size();
    for (auto& texture : scene_data.textures)
    {
        m_textures.push_back(std::make_unique<texture_2d>(texture));
    }

    std::size_t geometry_offset = m_geometries.size();
    for (const auto& geometry_data : scene_data.geometries)
    {
        auto model_geometry = std::make_unique<geometry>();

        model_geometry->set_positions(geometry_data.positions);
        model_geometry->set_normals(geometry_data.normals);
        model_geometry->set_tangents(geometry_data.tangents);
        model_geometry->set_texcoords(geometry_data.texcoords);
        model_geometry->set_indexes(geometry_data.indexes);

        for (const auto& submesh_data : geometry_data.submeshes)
        {
            if (submesh_data.clusters.empty())
            {
                model_geometry->add_submesh(
                    submesh_data.vertex_offset,
                    submesh_data.index_offset,
                    submesh_data.index_count);
            }
            else
            {
                model_geometry->add_submesh(submesh_data.clusters, submesh_data.cluster_nodes);
            }
        }

        m_geometries.push_back(std::move(model_geometry));
    }

    std::size_t material_offset = m_materials.size();
    for (const auto& material_data : scene_data.materials)
    {
        auto model_material = std::make_unique<pbr_material>();
        model_material->set_cull_mode(material_data.cull_mode);
        model_material->set_opacity_cutoff(material_data.opacity_cutoff);
        model_material->set_albedo(material_data.albedo);
        model_material->set_roughness(material_data.roughness);
        model_material->set_metallic(material_data.metallic);
        model_material->set_emissive(material_data.emissive);

        if (material_data.albedo_texture != -1)
        {
            model_material->set_albedo(
                m_textures[material_data.albedo_texture + texture_offset].get());
        }

        if (material_data.roughness_metallic_texture != -1)
        {
            model_material->set_roughness_metallic(
                m_textures[material_data.roughness_metallic_texture + texture_offset].get());
        }

        if (material_data.emissive_texture != -1)
        {
            model_material->set_emissive(
                m_textures[material_data.emissive_texture + texture_offset].get());
        }

        if (material_data.normal_texture != -1)
        {
            model_material->set_normal(
                m_textures[material_data.normal_texture + texture_offset].get());
        }

        m_materials.push_back(std::move(model_material));
    }

    entity root = world.create();
    world.add_component<transform_component, scene_component>(root);

    std::vector<entity> entities;

    for (const auto& node_data : scene_data.nodes)
    {
        entity entity = world.create();
        world.add_component<transform_component, parent_component, scene_component>(entity);

        auto& transform = world.get_component<transform_component>(entity);
        transform.set_position(node_data.position);
        transform.set_rotation(node_data.rotation);
        transform.set_scale(node_data.scale);

        entities.push_back(entity);
    }

    for (std::size_t i = 0; i < scene_data.nodes.size(); ++i)
    {
        const auto& node_data = scene_data.nodes[i];
        auto entity = entities[i];

        if (node_data.mesh != -1)
        {
            world.add_component<mesh_component>(entity);

            const auto& mesh_data = scene_data.meshes[node_data.mesh];

            auto& entity_mesh = world.get_component<mesh_component>(entity);
            entity_mesh.geometry = m_geometries[mesh_data.geometry + geometry_offset].get();
            entity_mesh.flags |= options & LOAD_OPTION_DYNAMIC_MESH ? 0 : MESH_STATIC;

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

void sample_system::imgui_profiling(rdg_profiling* profiling)
{
    static float smoothed_fps = 0.0f;

    float delta_time = ImGui::GetIO().DeltaTime;
    float current_fps = 1.0f / delta_time;

    const float smoothing = 0.1f;
    smoothed_fps = smoothed_fps * (1.0f - smoothing) + current_fps * smoothing;

    if (ImGui::Begin("GPU Profiler"))
    {
        ImGui::Text("FPS: %.1f", smoothed_fps);

        if (ImGui::BeginTable(
                "ProfilerTable",
                3,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_Sortable))
        {
            ImGui::TableSetupColumn("Pass", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Time (ms)");
            ImGui::TableSetupColumn("Frame %");

            ImGui::TableHeadersRow();

            const auto& nodes = profiling->get_nodes();
            imgui_profiling_event(nodes, 0, nodes[0].time_ms, "");

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void sample_system::initialize_render()
{
    m_swapchain = render_device::instance().create_swapchain({
        .flags = RHI_TEXTURE_TRANSFER_DST | RHI_TEXTURE_RENDER_TARGET,
        .window_handle = get_system<window_system>().get_handle(),
    });

    auto default_material = std::make_unique<pbr_material>();
    default_material->set_albedo({1.0f, 1.0f, 1.0f});
    default_material->set_roughness(0.8f);
    default_material->set_metallic(0.2f);
    m_materials.push_back(std::move(default_material));
}

void sample_system::initialize_scene(std::string_view skybox_path)
{
    auto& world = get_world();

    m_sky = world.create();
    world.add_component<transform_component, light_component, scene_component>(m_sky);

    if (!skybox_path.empty())
    {
        world.add_component<skybox_component>(m_sky);
        auto& skybox = world.get_component<skybox_component>(m_sky);

        texture_data texture_data = {
            .format = RHI_FORMAT_R8G8B8A8_SRGB,
        };
        texture_tool::load(skybox_path, texture_data);

        skybox.texture = texture_loader::load(texture_data);
    }
    else
    {
        world.add_component<atmosphere_component>(m_sky);
    }

    auto& light_transform = world.get_component<transform_component>(m_sky);
    light_transform.set_rotation(
        quaternion::from_euler(vec3f{math::to_radians(145.0f), math::to_radians(45.0f), 0.0f}));

    auto& main_light = world.get_component<light_component>(m_sky);
    main_light.type = LIGHT_DIRECTIONAL;
    main_light.color = {.x = 10.0f, .y = 10.0f, .z = 10.0f};
    main_light.cast_shadow = true;

    m_camera = world.create();
    world.add_component<
        transform_component,
        camera_component,
        first_person_control_component,
        scene_component>(m_camera);

    auto& camera_transform = world.get_component<transform_component>(m_camera);
    camera_transform.set_position({0.0f, 0.0f, -10.0f});

    auto& main_camera = world.get_component<camera_component>(m_camera);
    main_camera.renderer = std::make_unique<deferred_renderer_imgui>();
    main_camera.render_target = m_swapchain.get();
    main_camera.background =
        skybox_path.empty() ? BACKGROUND_TYPE_ATMOSPHERE : BACKGROUND_TYPE_SKYBOX;
}
} // namespace violet