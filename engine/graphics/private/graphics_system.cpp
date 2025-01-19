#include "graphics/graphics_system.hpp"
#include "camera_system.hpp"
#include "components/camera_component.hpp"
#include "components/camera_meta_component.hpp"
#include "components/scene_component.hpp"
#include "environment_system.hpp"
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
    m_scene_manager = nullptr;
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
}

void graphics_system::end_frame()
{
    auto& device = render_device::instance();

    bool need_record = device.get_material_manager()->update();
    need_record = m_scene_manager->update() || need_record;

    if (need_record)
    {
        rhi_command* command = device.allocate_command();

        device.get_material_manager()->record(command);
        m_scene_manager->record(command);

        command->signal(m_update_fence.get(), ++m_update_fence_value);
        device.execute(command);
    }

    auto& skinning = get_system<skinning_system>();
    if (skinning.need_record())
    {
        rhi_command* command = device.allocate_command();
        skinning.morphing(command);
        skinning.skinning(command);
        device.execute(command);
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
        rhi_fence* finish_fence = render(camera, parameter, *m_scene_manager->get_scene(layer));
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
    render_camera render_camera = {
        .camera_parameter = camera_parameter,
    };

    std::vector<rhi_swapchain*> swapchains;

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
                    render_camera.render_targets.push_back(arg->get_texture());
                }
                else if constexpr (std::is_same_v<T, rhi_texture*>)
                {
                    render_camera.render_targets.push_back(arg);
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

    render_graph graph("Camera", m_allocator.get());

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

    render_context context(scene, render_camera);

    camera->renderer->render(graph, context);

    graph.compile();

    graph.record(command);
    device.execute(command);

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
} // namespace violet