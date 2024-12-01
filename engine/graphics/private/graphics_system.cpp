#include "graphics/graphics_system.hpp"
#include "components/camera_component.hpp"
#include "components/camera_meta_component.hpp"
#include "components/light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/mesh_meta_component.hpp"
#include "components/scene_component.hpp"
#include "components/skybox_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/material_manager.hpp"
#include "rhi_plugin.hpp"

namespace violet
{
graphics_system::graphics_system()
    : engine_system("graphics")
{
}

graphics_system::~graphics_system()
{
    m_fences.clear();
    m_frame_fence = nullptr;
    m_allocator = nullptr;
    render_device::instance().reset();
    m_plugin->unload();
}

bool graphics_system::initialize(const dictionary& config)
{
    m_plugin = std::make_unique<rhi_plugin>();
    m_plugin->load(config["rhi"]);

    if (!m_plugin->get_rhi()->initialize({
            .features = RHI_FEATURE_INDIRECT_DRAW | RHI_FEATURE_BINDLESS,
            .frame_resource_count = config["frame_resource_count"],
        }))
    {
        return false;
    }

    render_device::instance().initialize(m_plugin->get_rhi());

    task_graph& task_graph = get_task_graph();
    task_group& pre_update_group = task_graph.get_group("PreUpdate Group");
    task_group& post_update_group = task_graph.get_group("Post Update Group");

    task& update_window_task = task_graph.get_task("Update Window");
    task& update_transform_task = task_graph.get_task("Update Transform");

    task_graph.add_task()
        .set_name("Frame Begin")
        .set_group(pre_update_group)
        .add_dependency(update_window_task)
        .set_execute(
            [this]()
            {
                begin_frame();
            });

    task& update_mesh_task = task_graph.add_task();
    update_mesh_task.set_name("Update Mesh")
        .set_group(post_update_group)
        .add_dependency(update_transform_task)
        .set_execute(
            [this]()
            {
                update_mesh();
            });

    task& update_camera_task = task_graph.add_task();
    update_camera_task.set_name("Update Camera")
        .set_group(post_update_group)
        .add_dependency(update_transform_task)
        .set_execute(
            [this]()
            {
                udpate_camera();
            });

    task_graph.add_task()
        .set_name("Frame End")
        .set_group(post_update_group)
        .add_dependency(update_mesh_task, update_camera_task)
        .set_execute(
            [this]()
            {
                end_frame();
                m_system_version = get_world().get_version();
            });

    auto& world = get_world();
    world.register_component<mesh_component>();
    world.register_component<mesh_meta_component>();
    world.register_component<camera_component>();
    world.register_component<camera_meta_component>();
    world.register_component<light_component>();
    world.register_component<skybox_component>();

    m_allocator = std::make_unique<rdg_allocator>();

    auto& device = render_device::instance();
    m_used_fences.resize(device.get_frame_resource_count());
    m_frame_fence_values.resize(device.get_frame_resource_count());

    m_frame_fence = device.create_fence();
    m_update_fence = device.create_fence();

    return true;
}

void graphics_system::shutdown() {}

void graphics_system::udpate_camera()
{
    auto& world = get_world();

    world.get_view()
        .read<camera_component>()
        .read<transform_world_component>()
        .write<camera_meta_component>()
        .each(
            [](const camera_component& camera,
               const transform_world_component& transform,
               camera_meta_component& camera_meta)
            {
                if (camera.render_targets.empty())
                {
                    return;
                }

                shader::camera_data data = {};

                rhi_texture_extent extent = camera.get_extent();
                float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

                mat4f_simd projection = matrix::perspective_reverse_z<simd>(
                    camera.fov,
                    aspect,
                    camera.near,
                    camera.far);
                mat4f_simd view = matrix::inverse(math::load(transform.matrix));
                mat4f_simd view_projection = matrix::mul(view, projection);

                math::store(projection, data.projection);
                math::store(view, data.view);
                math::store(view_projection, data.view_projection);

                data.position = transform.get_position();

                if (camera_meta.parameter == nullptr)
                {
                    camera_meta.parameter =
                        render_device::instance().create_parameter(shader::camera);
                }

                camera_meta.parameter->set_constant(0, &data, sizeof(shader::camera_data), 0);
            },
            [this](auto& view)
            {
                return view.is_updated<camera_component>(m_system_version) ||
                       view.is_updated<transform_world_component>(m_system_version);
            });
}

void graphics_system::update_mesh()
{
    auto& world = get_world();

    world.get_view()
        .read<scene_component>()
        .read<mesh_component>()
        .read<transform_world_component>()
        .write<mesh_meta_component>()
        .each(
            [this](
                const scene_component& scene,
                const mesh_component& mesh,
                const transform_world_component& transform,
                mesh_meta_component& mesh_meta)
            {
                render_scene* render_scene = get_scene(scene.layer);
                if (mesh_meta.scene == render_scene)
                {
                    return;
                }

                if (mesh_meta.scene != nullptr)
                {
                    for (render_id instance : mesh_meta.instances)
                    {
                        mesh_meta.scene->remove_instance(instance);
                    }

                    mesh_meta.scene->remove_mesh(mesh_meta.mesh);
                }

                mesh_meta.scene = render_scene;

                mesh_meta.mesh = render_scene->add_mesh({
                    .model_matrix = transform.matrix,
                    .geometry = mesh.geometry,
                });

                for (auto& submesh : mesh.submeshes)
                {
                    render_id instance = render_scene->add_instance(
                        mesh_meta.mesh,
                        {
                            .vertex_offset = submesh.vertex_offset,
                            .index_offset = submesh.index_offset,
                            .index_count = submesh.index_count,
                            .material = submesh.material,
                        });
                    mesh_meta.instances.push_back(instance);
                }

                mesh_meta.scene = render_scene;
            },
            [this](auto& view)
            {
                return view.is_updated<scene_component>(m_system_version);
            });

    world.get_view().read<mesh_component>().write<mesh_meta_component>().each(
        [](const mesh_component& mesh, mesh_meta_component& mesh_meta)
        {
            if (mesh_meta.scene == nullptr)
            {
                return;
            }

            std::size_t instance_count =
                std::min(mesh.submeshes.size(), mesh_meta.instances.size());

            for (std::size_t i = 0; i < instance_count; ++i)
            {
                mesh_meta.scene->update_instance(
                    mesh_meta.instances[i],
                    {
                        .vertex_offset = mesh.submeshes[i].vertex_offset,
                        .index_offset = mesh.submeshes[i].index_offset,
                        .index_count = mesh.submeshes[i].index_count,
                        .material = mesh.submeshes[i].material,
                    });
            }

            for (std::size_t i = instance_count; i < mesh.submeshes.size(); ++i)
            {
                render_id instance_id = mesh_meta.scene->add_instance(
                    mesh_meta.mesh,
                    {
                        .vertex_offset = mesh.submeshes[i].vertex_offset,
                        .index_offset = mesh.submeshes[i].index_offset,
                        .index_count = mesh.submeshes[i].index_count,
                        .material = mesh.submeshes[i].material,
                    });
                mesh_meta.instances.push_back(instance_id);
            }

            while (mesh_meta.instances.size() > instance_count)
            {
                mesh_meta.scene->remove_instance(mesh_meta.instances.back());
                mesh_meta.instances.pop_back();
            }
        },
        [this](auto& view)
        {
            return view.is_updated<mesh_component>(m_system_version);
        });

    world.get_view().read<transform_world_component>().write<mesh_meta_component>().each(
        [](const transform_world_component& transform, mesh_meta_component& mesh_meta)
        {
            if (mesh_meta.scene != nullptr)
            {
                mesh_meta.scene->update_mesh_model_matrix(mesh_meta.mesh, transform.matrix);
            }
        },
        [this](auto& view)
        {
            return view.is_updated<transform_world_component>(m_system_version);
        });
}

void graphics_system::update_environment()
{
    auto& world = get_world();

    world.get_view().read<skybox_component>().read<scene_component>().each(
        [this](const skybox_component& skybox, const scene_component& scene)
        {
            render_scene* render_scene = get_scene(scene.layer);
            // render_scene->set_skybox(
            //     skybox.texture,
            //     m_skybox_sampler.get(),
            //     skybox.irradiance,
            //     m_skybox_sampler.get(),
            //     skybox.prefilter,
            //     m_prefilter_sampler.get());
        },
        [this](auto& view)
        {
            return view.is_updated<skybox_component>(m_system_version);
        });
}

void graphics_system::begin_frame()
{
    auto& device = render_device::instance();

    ++m_frame_fence_value;

    m_frame_fence->wait(m_frame_fence_values[device.get_frame_resource_index()]);

    device.begin_frame();
    switch_frame_resource();
    m_allocator->reset();
}

void graphics_system::end_frame()
{
    auto& device = render_device::instance();

    bool need_record = device.get_material_manager()->update();

    for (auto& scene : m_scenes)
    {
        need_record = scene->update() || need_record;
    }

    if (need_record)
    {
        rhi_command* update_command = device.allocate_command();

        device.get_material_manager()->record(update_command);

        for (auto& scene : m_scenes)
        {
            scene->record(update_command);
        }

        update_command->signal(m_update_fence.get(), ++m_update_fence_value);
        device.execute(update_command);
    }

    render();

    device.end_frame();
}

void graphics_system::render()
{
    auto& world = get_world();

    std::vector<std::tuple<const camera_component*, rhi_parameter*, std::uint32_t>> render_queue;

    world.get_view()
        .read<camera_component>()
        .read<camera_meta_component>()
        .read<scene_component>()
        .each(
            [&render_queue](
                const camera_component& camera,
                const camera_meta_component& camera_meta,
                const scene_component& scene)
            {
                render_queue.push_back({&camera, camera_meta.parameter.get(), scene.layer});
            });

    std::sort(
        render_queue.begin(),
        render_queue.end(),
        [&world](auto& a, auto& b)
        {
            return std::get<0>(a)->priority > std::get<0>(b)->priority;
        });

    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();

    for (auto& [camera, parameter, layer] : render_queue)
    {
        rhi_fence* finish_fence = render(camera, parameter, *get_scene(layer));
        if (finish_fence)
        {
            command->wait(finish_fence, m_frame_fence_value);
        }
    }

    m_frame_fence_values[device.get_frame_resource_index()] = m_frame_fence_value;

    command->signal(m_frame_fence.get(), m_frame_fence_value);

    device.execute(command);
}

rhi_fence* graphics_system::render(
    const camera_component* camera,
    rhi_parameter* camera_parameter,
    const render_scene& scene)
{
    std::vector<rhi_swapchain*> swapchains;
    std::vector<rhi_texture*> render_targets;

    auto& device = render_device::instance();
    rhi_command* command = device.allocate_command();

    command->wait(m_update_fence.get(), m_update_fence_value);

    for (auto& render_target : camera->render_targets)
    {
        bool skip = false;

        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, rhi_swapchain*>)
                {
                    rhi_fence* fence = arg->acquire_texture();
                    if (fence == nullptr)
                    {
                        skip = true;
                        return;
                    }

                    command->wait(fence, 0, RHI_PIPELINE_STAGE_BEGIN);

                    swapchains.push_back(arg);
                    render_targets.push_back(arg->get_texture());
                }
                else if constexpr (std::is_same_v<T, rhi_texture*>)
                {
                    render_targets.push_back(arg);
                }
            },
            render_target);

        if (skip)
        {
            return nullptr;
        }
    }

    for (std::size_t i = 0; i < swapchains.size(); ++i)
    {
        command->signal(swapchains[i]->get_present_fence(), m_frame_fence_value);
    }
    rhi_fence* finish_fence = allocate_fence();
    command->signal(finish_fence, m_frame_fence_value);

    render_graph graph(m_allocator.get());

    rhi_texture_extent extent = camera->get_extent();
    camera->renderer->render(
        graph,
        scene,
        {
            .camera_parameter = camera_parameter,
            .render_targets = render_targets,
            .viewport =
                {
                    .x = 0.0f,
                    .y = 0.0f,
                    .width = static_cast<float>(extent.width),
                    .height = static_cast<float>(extent.height),
                    .min_depth = 0.0f,
                    .max_depth = 1.0f,
                },
            .scissor_rect =
                {
                    .min_x = 0,
                    .min_y = 0,
                    .max_x = extent.width,
                    .max_y = extent.height,
                },
        });

    graph.compile();
    graph.record(command);

    device.execute(command);

    for (std::size_t i = 0; i < swapchains.size(); ++i)
    {
        swapchains[i]->present();
    }

    return finish_fence;
}

void graphics_system::switch_frame_resource()
{
    auto& device = render_device::instance();
    auto& finish_fences = m_used_fences[device.get_frame_resource_index()];
    for (rhi_fence* samphore : finish_fences)
    {
        m_free_fences.push_back(samphore);
    }
    finish_fences.clear();
}

rhi_fence* graphics_system::allocate_fence()
{
    auto& device = render_device::instance();

    if (m_free_fences.empty())
    {
        m_fences.emplace_back(device.create_fence());
        m_free_fences.push_back(m_fences.back().get());
    }

    rhi_fence* fence = m_free_fences.back();
    m_used_fences[device.get_frame_resource_index()].push_back(fence);

    m_free_fences.pop_back();

    return fence;
}

render_scene* graphics_system::get_scene(std::uint32_t layer)
{
    assert(layer < 64);

    while (layer >= m_scenes.size())
    {
        m_scenes.push_back(std::make_unique<render_scene>());
    }

    return m_scenes[layer].get();
}
} // namespace violet