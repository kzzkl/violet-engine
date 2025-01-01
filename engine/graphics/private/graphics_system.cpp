#include "graphics/graphics_system.hpp"
#include "components/camera_component.hpp"
#include "components/camera_meta_component.hpp"
#include "components/light_component.hpp"
#include "components/light_meta_component.hpp"
#include "components/mesh_component.hpp"
#include "components/mesh_meta_component.hpp"
#include "components/morph_component.hpp"
#include "components/scene_component.hpp"
#include "components/skeleton_component.hpp"
#include "components/skinned_component.hpp"
#include "components/skinned_meta_component.hpp"
#include "components/skybox_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/material_manager.hpp"
#include "math/matrix.hpp"
#include "rhi_plugin.hpp"

namespace violet
{
graphics_system::graphics_system()
    : engine_system("graphics")
{
}

graphics_system::~graphics_system()
{
    m_scenes.clear();
    m_fences.clear();
    m_frame_fence = nullptr;
    m_update_fence = nullptr;
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

    auto& task_graph = get_task_graph();
    auto& pre_update_group = task_graph.get_group("PreUpdate");
    auto& update_window_task = task_graph.get_task("Update Window");

    task_graph.add_task()
        .set_name("Frame Begin")
        .set_group(pre_update_group)
        .add_dependency(update_window_task)
        .set_execute(
            [this]()
            {
                begin_frame();
            });

    auto& post_update_group = task_graph.get_group("PostUpdate");
    auto& transform_group = task_graph.get_group("Transform");
    auto& rendering_group = task_graph.add_group();
    rendering_group.set_name("Rendering")
        .set_group(post_update_group)
        .add_dependency(transform_group);

    auto& update_scene_task = task_graph.add_task();
    update_scene_task.set_name("Update Scene")
        .set_group(rendering_group)
        .set_execute(
            [this]()
            {
                update_mesh();
                update_skin();
                update_skeleton();
                update_light();
                update_environment();
            });

    auto& update_camera_task = task_graph.add_task();
    update_camera_task.set_name("Update Camera")
        .set_group(rendering_group)
        .set_execute(
            [this]()
            {
                udpate_camera();
            });

    task_graph.add_task()
        .set_name("Frame End")
        .set_group(rendering_group)
        .add_dependency(update_scene_task, update_camera_task)
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
    world.register_component<light_meta_component>();
    world.register_component<skybox_component>();
    world.register_component<skinned_component>();
    world.register_component<skinned_meta_component>();
    world.register_component<skeleton_component>();
    world.register_component<morph_component>();

    m_allocator = std::make_unique<rdg_allocator>();

    auto& device = render_device::instance();
    m_used_fences.resize(device.get_frame_resource_count());
    m_frame_fence_values.resize(device.get_frame_resource_count());

    m_frame_fence = device.create_fence();
    m_update_fence = device.create_fence();

    return true;
}

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

                shader::camera_data data = {
                    .position = transform.get_position(),
                    .fov = camera.fov,
                };

                rhi_texture_extent extent = camera.get_extent();
                float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

                mat4f_simd projection = matrix::perspective_reverse_z<simd>(
                    camera.fov,
                    aspect,
                    camera.near,
                    camera.far);
                mat4f_simd view = matrix::inverse(math::load(transform.matrix));
                mat4f_simd view_projection = matrix::mul(view, projection);
                mat4f_simd view_projection_inv = matrix::inverse(view_projection);

                math::store(projection, data.projection);
                math::store(view, data.view);
                math::store(view_projection, data.view_projection);
                math::store(view_projection_inv, data.view_projection_inv);

                if (camera_meta.parameter == nullptr)
                {
                    camera_meta.parameter =
                        render_device::instance().create_parameter(shader::camera);
                }

                camera_meta.parameter->set_constant(0, &data, sizeof(shader::camera_data));
            },
            [this](auto& view)
            {
                return view.template is_updated<camera_component>(m_system_version) ||
                       view.template is_updated<transform_world_component>(m_system_version);
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
                if (mesh_meta.scene != render_scene)
                {
                    if (mesh_meta.scene != nullptr)
                    {
                        for (render_id instance : mesh_meta.instances)
                        {
                            mesh_meta.scene->remove_instance(instance);
                        }

                        mesh_meta.scene->remove_mesh(mesh_meta.mesh);
                    }

                    mesh_meta.mesh = render_scene->add_mesh();
                    render_scene->set_mesh_geometry(mesh_meta.mesh, mesh.geometry);

                    mesh_meta.instances.clear();
                    mesh_meta.scene = render_scene;
                }

                render_scene->set_mesh_model_matrix(mesh_meta.mesh, transform.matrix);
            },
            [this](auto& view)
            {
                return view.template is_updated<scene_component>(m_system_version) ||
                       view.template is_updated<transform_world_component>(m_system_version);
            });

    world.get_view().read<mesh_component>().write<mesh_meta_component>().each(
        [](const mesh_component& mesh, mesh_meta_component& mesh_meta)
        {
            if (mesh_meta.scene == nullptr)
            {
                return;
            }

            mesh_meta.scene->set_mesh_geometry(mesh_meta.mesh, mesh.geometry);

            std::size_t instance_count =
                std::min(mesh.submeshes.size(), mesh_meta.instances.size());

            for (std::size_t i = 0; i < instance_count; ++i)
            {
                mesh_meta.scene->set_instance_data(
                    mesh_meta.instances[i],
                    mesh.submeshes[i].vertex_offset,
                    mesh.submeshes[i].index_offset,
                    mesh.submeshes[i].index_count);
                mesh_meta.scene->set_instance_material(
                    mesh_meta.instances[i],
                    mesh.submeshes[i].material);
            }

            for (std::size_t i = instance_count; i < mesh.submeshes.size(); ++i)
            {
                render_id instance = mesh_meta.scene->add_instance(mesh_meta.mesh);
                mesh_meta.scene->set_instance_data(
                    instance,
                    mesh.submeshes[i].vertex_offset,
                    mesh.submeshes[i].index_offset,
                    mesh.submeshes[i].index_count);
                mesh_meta.scene->set_instance_material(instance, mesh.submeshes[i].material);
                mesh_meta.instances.push_back(instance);
            }

            while (mesh_meta.instances.size() > mesh.submeshes.size())
            {
                mesh_meta.scene->remove_instance(mesh_meta.instances.back());
                mesh_meta.instances.pop_back();
            }
        },
        [this](auto& view)
        {
            return view.template is_updated<mesh_component>(m_system_version);
        });
}

void graphics_system::update_skin()
{
    auto& world = get_world();

    world.get_view()
        .read<mesh_component>()
        .read<mesh_meta_component>()
        .read<skinned_component>()
        .write<skinned_meta_component>()
        .each(
            [](const mesh_component& mesh,
               const mesh_meta_component& mesh_meta,
               const skinned_component& skinned,
               skinned_meta_component& skinned_meta)
            {
                if (skinned_meta.skinned_geometry == nullptr)
                {
                    skinned_meta.skinned_geometry = std::make_unique<geometry>();
                }

                if (mesh.geometry != skinned_meta.original_geometry)
                {
                    skinned_meta.original_geometry = mesh.geometry;

                    std::map<std::string, rhi_buffer*> buffers;
                    for (const auto& [name, buffer] : mesh.geometry->get_vertex_buffers())
                    {
                        buffers[name] = buffer;
                    }

                    for (const auto& [name, format] : skinned.outputs)
                    {
                        skinned_meta.skinned_geometry->add_attribute(
                            name,
                            {
                                .size = rhi_get_format_stride(format) *
                                        mesh.geometry->get_vertex_count(),
                                .flags = RHI_BUFFER_VERTEX | RHI_BUFFER_STORAGE,
                            });

                        auto iter = buffers.find(name);
                        if (iter != buffers.end())
                        {
                            buffers.erase(iter);
                        }
                    }

                    if (mesh.geometry->get_morph_target_count() != 0)
                    {
                        skinned_meta.skinned_geometry->add_attribute(
                            "morph",
                            {
                                .size = sizeof(vec3f) * mesh.geometry->get_vertex_count(),
                                .flags = RHI_BUFFER_TRANSFER_DST | RHI_BUFFER_STORAGE |
                                         RHI_BUFFER_UNIFORM_TEXEL,
                                .texel =
                                    {
                                        .format = RHI_FORMAT_R32_SINT,
                                    },
                            });
                    }

                    for (const auto& [name, buffer] : buffers)
                    {
                        skinned_meta.skinned_geometry->add_attribute(
                            name,
                            mesh.geometry->get_vertex_buffer(name));
                    }

                    skinned_meta.skinned_geometry->set_indexes(mesh.geometry->get_index_buffer());

                    skinned_meta.skinned_geometry->set_vertex_count(
                        mesh.geometry->get_vertex_count());
                    skinned_meta.skinned_geometry->set_index_count(
                        mesh.geometry->get_index_count());
                }

                if (mesh_meta.scene != nullptr)
                {
                    mesh_meta.scene->set_mesh_geometry(
                        mesh_meta.mesh,
                        skinned_meta.skinned_geometry.get());
                }
            },
            [this](auto& view)
            {
                return view.template is_updated<skinned_component>(m_system_version);
            });
}

void graphics_system::update_skeleton()
{
    auto& world = get_world();

    world.get_view().read<skeleton_component>().write<skinned_meta_component>().each(
        [&world](const skeleton_component& skeleton, skinned_meta_component& skinned_meta)
        {
            if (skeleton.bones.empty())
            {
                return;
            }

            if (skinned_meta.bone_buffers.empty())
            {
                auto& device = render_device::instance();

                std::size_t frame_resource_count = device.get_frame_resource_count();
                for (std::size_t i = 0; i < frame_resource_count; ++i)
                {
                    rhi_buffer_desc desc = {
                        .size = sizeof(mat4f) * skeleton.bones.size(),
                        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_HOST_VISIBLE,
                    };
                    skinned_meta.bone_buffers.push_back(device.create_buffer(desc));
                }

                skinned_meta.current_index = 0;
            }
            else
            {
                skinned_meta.current_index =
                    (skinned_meta.current_index + 1) % skinned_meta.bone_buffers.size();
            }

            auto* buffer = static_cast<mat4f*>(
                skinned_meta.bone_buffers[skinned_meta.current_index]->get_buffer_pointer());
            for (const auto& bone : skeleton.bones)
            {
                const auto& bone_transform =
                    world.get_component<const transform_world_component>(bone.entity);
                mat4f_simd bone_matrix = matrix::mul(
                    math::load(bone.binding_pose_inv),
                    math::load(bone_transform.matrix));

                math::store(bone_matrix, *buffer);

                ++buffer;
            }
        });
}

void graphics_system::update_light()
{
    auto& world = get_world();

    world.get_view()
        .read<scene_component>()
        .read<light_component>()
        .read<transform_world_component>()
        .write<light_meta_component>()
        .each(
            [this](
                const scene_component& scene,
                const light_component& light,
                const transform_world_component& transform,
                light_meta_component& light_meta)
            {
                render_scene* render_scene = get_scene(scene.layer);

                if (light_meta.scene != render_scene)
                {
                    if (light_meta.scene != nullptr)
                    {
                        render_scene->remove_light(light_meta.id);
                    }

                    light_meta.id = render_scene->add_light();
                    light_meta.scene = render_scene;
                }

                render_scene->set_light_data(
                    light_meta.id,
                    light.type,
                    light.color,
                    transform.get_position(),
                    transform.get_forward());
            },
            [this](auto& view)
            {
                return view.template is_updated<light_component>(m_system_version) ||
                       view.template is_updated<scene_component>(m_system_version) ||
                       view.template is_updated<transform_world_component>(m_system_version);
            });
}

void graphics_system::update_environment()
{
    auto& world = get_world();

    world.get_view().read<skybox_component>().read<scene_component>().each(
        [this](const skybox_component& skybox, const scene_component& scene)
        {
            render_scene* render_scene = get_scene(scene.layer);
            render_scene->set_skybox(skybox.texture, skybox.irradiance, skybox.prefilter);
        },
        [this](auto& view)
        {
            return view.template is_updated<skybox_component>(m_system_version);
        });
}

void graphics_system::morphing()
{
    auto& world = get_world();
    auto& device = render_device::instance();

    struct morphing_data
    {
        morph_target_buffer* morph_target_buffer;
        const float* weights;
        std::size_t weight_count;

        rhi_buffer* morph_vertex_buffer;
    };
    std::vector<morphing_data> morphing_queue;

    world.get_view().write<skinned_meta_component>().read<morph_component>().each(
        [&](skinned_meta_component& skinned_meta, const morph_component& morph)
        {
            morph_target_buffer* morph_target_buffer =
                skinned_meta.original_geometry->get_morph_target_buffer();

            if (morph_target_buffer == nullptr)
            {
                return;
            }

            morphing_data data = {
                .morph_target_buffer = morph_target_buffer,
                .weights = morph.weights.data(),
                .weight_count = morph.weights.size(),
                .morph_vertex_buffer = skinned_meta.skinned_geometry->get_vertex_buffer("morph"),
            };
            morphing_queue.push_back(data);
        });

    if (morphing_queue.empty())
    {
        return;
    }

    rhi_command* command = device.allocate_command();

    command->begin_label("morphing");

    command->set_pipeline(m_allocator->get_pipeline({
        .compute_shader = device.get_shader<morphing_cs>(),
    }));
    command->set_parameter(0, device.get_bindless_parameter());

    for (auto& morphing_data : morphing_queue)
    {
        morphing_data.morph_target_buffer->update_morph(
            command,
            m_allocator.get(),
            morphing_data.morph_vertex_buffer,
            std::span(morphing_data.weights, morphing_data.weight_count));
    }

    command->end_label();

    device.execute(command);
}

void graphics_system::skinning()
{
    auto& world = get_world();
    auto& device = render_device::instance();

    struct skinning_data
    {
        rhi_shader* shader;
        std::size_t vertex_count;

        rhi_buffer* skeleton;
        std::vector<rhi_buffer*> input;
        std::vector<rhi_buffer*> output;
    };
    std::vector<skinning_data> skinning_queue;

    world.get_view().read<skinned_component>().read<skinned_meta_component>().each(
        [&](const skinned_component& skinned, const skinned_meta_component& skinned_meta)
        {
            if (skinned_meta.bone_buffers.empty())
            {
                return;
            }

            skinning_data data = {
                .shader = skinned.shader,
                .vertex_count = skinned_meta.skinned_geometry->get_vertex_count(),
                .skeleton = skinned_meta.bone_buffers[skinned_meta.current_index].get(),
            };

            data.input.reserve(skinned.inputs.size());
            for (const auto& input : skinned.inputs)
            {
                if (input == "morph")
                {
                    data.input.push_back(skinned_meta.skinned_geometry->get_vertex_buffer(input));
                }
                else
                {
                    data.input.push_back(skinned_meta.original_geometry->get_vertex_buffer(input));
                }
            }

            data.output.reserve(skinned.outputs.size());
            for (const auto& [name, format] : skinned.outputs)
            {
                data.output.push_back(skinned_meta.skinned_geometry->get_vertex_buffer(name));
            }

            skinning_queue.push_back(data);
        });

    if (skinning_queue.empty())
    {
        return;
    }

    rhi_command* command = device.allocate_command();

    command->begin_label("skinning");

    for (const auto& skinning_data : skinning_queue)
    {
        skinning_cs::skinning_data skinning_constant = {
            .skeleton = skinning_data.skeleton->get_handle(),
        };
        std::size_t buffer_index = 0;
        for (auto* input : skinning_data.input)
        {
            skinning_constant.buffers[buffer_index++] = input->get_handle();
        }

        std::vector<rhi_buffer_barrier> buffer_barriers;
        buffer_barriers.reserve(skinning_data.output.size());
        for (auto* output : skinning_data.output)
        {
            skinning_constant.buffers[buffer_index++] = output->get_handle();
            buffer_barriers.push_back({
                .buffer = output,
                .src_access = RHI_ACCESS_SHADER_WRITE,
                .dst_access = RHI_ACCESS_VERTEX_ATTRIBUTE_READ,
                .offset = 0,
                .size = output->get_buffer_size(),
            });
        }

        rhi_parameter* parameter = m_allocator->allocate_parameter(skinning_cs::skinning);
        parameter->set_constant(0, &skinning_constant, sizeof(skinning_cs::skinning_data));

        command->set_pipeline(m_allocator->get_pipeline(rdg_compute_pipeline{
            .compute_shader = skinning_data.shader,
        }));
        command->set_parameter(0, device.get_bindless_parameter());
        command->set_parameter(1, parameter);
        command->dispatch((skinning_data.vertex_count + 63) / 64, 1, 1);

        command->set_pipeline_barrier(
            RHI_PIPELINE_STAGE_COMPUTE,
            RHI_PIPELINE_STAGE_VERTEX_INPUT,
            buffer_barriers.data(),
            buffer_barriers.size(),
            nullptr,
            0);
    }

    command->end_label();

    device.execute(command);
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

    morphing();
    skinning();
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
                render_queue.emplace_back(&camera, camera_meta.parameter.get(), scene.layer);
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

    for (const auto& render_target : camera->render_targets)
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

    for (auto& swapchain : swapchains)
    {
        command->signal(swapchain->get_present_fence(), m_frame_fence_value);
    }
    rhi_fence* finish_fence = allocate_fence();
    command->signal(finish_fence, m_frame_fence_value);

    render_graph graph(m_allocator.get());

    render_camera render_camera = {
        .camera_parameter = camera_parameter,
        .render_targets = render_targets,
    };

    rhi_texture_extent extent = camera->get_extent();
    if (camera->viewport.width == 0.0f || camera->viewport.height == 0.0f)
    {
        render_camera.viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(extent.width),
            .height = static_cast<float>(extent.height),
            .min_depth = 0.0f,
            .max_depth = 1.0f,
        };
    }

    if (camera->scissor_rects.empty())
    {
        render_camera.scissor_rects.emplace_back(rhi_scissor_rect{
            .min_x = 0,
            .min_y = 0,
            .max_x = extent.width,
            .max_y = extent.height,
        });
    }
    else
    {
        render_camera.scissor_rects = camera->scissor_rects;
    }

    camera->renderer->render(graph, scene, render_camera);

    graph.compile();

    command->begin_label("Camera");

    graph.record(command);
    device.execute(command);

    command->end_label();

    for (auto& swapchain : swapchains)
    {
        swapchain->present();
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