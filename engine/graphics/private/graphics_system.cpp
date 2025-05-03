#include "graphics/graphics_system.hpp"
#include "camera_system.hpp"
#include "components/camera_component.hpp"
#include "components/camera_component_meta.hpp"
#include "components/scene_component.hpp"
#include "environment_system.hpp"
#include "gpu_buffer_uploader.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/material_manager.hpp"
#include "light_system.hpp"
#include "mesh_system.hpp"
#include "render_scene_manager.hpp"
#include "rhi_plugin.hpp"
#include "scene/scene_system.hpp"
#include "skinning_system.hpp"

namespace violet
{
graphics_system::graphics_system()
    : system("graphics")
{
}

graphics_system::~graphics_system()
{
#ifndef NDEBUG
    m_debug_drawer = nullptr;
#endif
    m_scene_manager = nullptr;
    m_gpu_buffer_uploader = nullptr;
    m_fences.clear();
    m_frame_fence = nullptr;
    m_update_fence = nullptr;
    m_allocator = nullptr;
    render_device::instance().reset();
    m_plugin->unload();
}

void graphics_system::install(application& app)
{
    app.install<scene_system>();
    app.install<camera_system>();
    app.install<mesh_system>();
    app.install<light_system>();
    app.install<skinning_system>();
    app.install<environment_system>();
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
#ifndef NDEBUG
                m_debug_drawer->tick();
#endif

                get_system<mesh_system>().update(*m_scene_manager);
                get_system<skinning_system>().update();
                get_system<light_system>().update(*m_scene_manager);
                get_system<environment_system>().update(*m_scene_manager);
            });

    auto& update_camera_task = task_graph.add_task();
    update_camera_task.set_name("Update Camera")
        .set_group(rendering_group)
        .set_execute(
            [this]()
            {
                get_system<camera_system>().update();
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

    m_allocator = std::make_unique<rdg_allocator>();

    auto& device = render_device::instance();
    m_used_fences.resize(device.get_frame_resource_count());
    m_frame_fence_values.resize(device.get_frame_resource_count());

    m_frame_fence = device.create_fence();
    m_update_fence = device.create_fence();

    m_scene_manager = std::make_unique<render_scene_manager>();

    m_gpu_buffer_uploader = std::make_unique<gpu_buffer_uploader>(4ull * 1024 * 1024);

#ifndef NDEBUG
    m_debug_drawer = std::make_unique<debug_drawer>(get_world());
#endif

    return true;
}

void graphics_system::begin_frame()
{
    auto& device = render_device::instance();

    ++m_frame_fence_value;

    m_frame_fence->wait(m_frame_fence_values[device.get_frame_resource_index()]);

    device.begin_frame();
    switch_frame_resource();
    m_allocator->tick();
    m_gpu_buffer_uploader->tick();
}

void graphics_system::end_frame()
{
    auto& device = render_device::instance();

    device.get_material_manager()->update(m_gpu_buffer_uploader.get());
    device.get_geometry_manager()->update(m_gpu_buffer_uploader.get());
    m_scene_manager->update(m_gpu_buffer_uploader.get());

    rhi_command* command = nullptr;

    if (!m_gpu_buffer_uploader->empty())
    {
        if (command == nullptr)
        {
            command = device.allocate_command();
        }

        m_gpu_buffer_uploader->record(command);
    }

    auto& skinning = get_system<skinning_system>();
    if (skinning.need_record())
    {
        if (command == nullptr)
        {
            command = device.allocate_command();
        }

        skinning.morphing(command);
        skinning.skinning(command);
    }

    if (command != nullptr)
    {
        command->signal(m_update_fence.get(), ++m_update_fence_value);
        device.execute(command);
    }

    render();

    device.end_frame();
}

void graphics_system::render()
{
    auto& world = get_world();

    std::vector<render_context> render_queue;

    world.get_view()
        .read<camera_component>()
        .read<camera_component_meta>()
        .read<scene_component>()
        .each(
            [&render_queue](
                const camera_component& camera,
                const camera_component_meta& camera_meta,
                const scene_component& scene)
            {
                render_queue.emplace_back(&camera, &camera_meta, scene.layer);
            });

    std::sort(
        render_queue.begin(),
        render_queue.end(),
        [&world](auto& a, auto& b)
        {
            return a.camera->priority > b.camera->priority;
        });

    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();

    for (const auto& context : render_queue)
    {
        rhi_fence* finish_fence = render(context);
        if (finish_fence)
        {
            command->wait(finish_fence, m_frame_fence_value);
        }
    }

    m_frame_fence_values[device.get_frame_resource_index()] = m_frame_fence_value;

    command->signal(m_frame_fence.get(), m_frame_fence_value);

    device.execute(command);
}

rhi_fence* graphics_system::render(const render_context& context)
{
    auto& device = render_device::instance();
    rhi_command* command = device.allocate_command();

    command->wait(m_update_fence.get(), m_update_fence_value);

    bool skip = false;

    rhi_swapchain* swapchain = nullptr;

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

                swapchain = arg;
            }
        },
        context.camera->render_target);

    if (skip)
    {
        return nullptr;
    }

    if (swapchain != nullptr)
    {
        command->signal(swapchain->get_present_fence(), m_frame_fence_value);
    }

    rhi_fence* finish_fence = allocate_fence();
    command->signal(finish_fence, m_frame_fence_value);

    render_camera render_camera(context.camera, context.camera_meta);
    render_graph graph(
        "Camera",
        m_scene_manager->get_scene(context.layer),
        &render_camera,
        m_allocator.get());

    render_camera.get_hzb();

    context.camera->renderer->render(graph);

    graph.compile();

    graph.record(command);
    device.execute(command);

    if (swapchain != nullptr)
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
} // namespace violet