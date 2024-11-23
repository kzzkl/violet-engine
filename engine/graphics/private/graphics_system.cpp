#include "graphics/graphics_system.hpp"
#include "components/camera.hpp"
#include "components/camera_render_data.hpp"
#include "components/light.hpp"
#include "components/mesh.hpp"
#include "components/scene_layer.hpp"
#include "components/transform.hpp"
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
    render_context::instance().reset();
    render_device::instance().reset();
    m_plugin->unload();
}

bool graphics_system::initialize(const dictionary& config)
{
    rhi_desc rhi_desc = {};
    rhi_desc.frame_resource_count = config["frame_resource_count"];

    m_plugin = std::make_unique<rhi_plugin>();
    m_plugin->load(config["rhi"]);
    m_plugin->get_rhi()->initialize(rhi_desc);

    render_device::instance().initialize(m_plugin->get_rhi());

    task_graph& task_graph = get_task_graph();
    task_group& pre_update = task_graph.get_group("PreUpdate Group");
    task_group& post_update = task_graph.get_group("Post Update Group");

    task& update_window = task_graph.get_task("Update Window");

    task_graph.add_task()
        .set_name("Frame Begin")
        .set_group(pre_update)
        .add_dependency(update_window)
        .set_execute(
            [this]()
            {
                begin_frame();
            });

    task& update_mesh = task_graph.get_task("Update Mesh");
    task& update_camera = task_graph.get_task("Update Camera");

    task_graph.add_task()
        .set_name("Frame End")
        .set_group(post_update)
        .add_dependency(update_mesh, update_camera)
        .set_execute(
            [this]()
            {
                end_frame();
            });

    auto& world = get_world();
    world.register_component<light>();

    m_allocator = std::make_unique<rdg_allocator>();

    auto& device = render_device::instance();
    m_used_fences.resize(device.get_frame_resource_count());
    m_frame_fence_values.resize(device.get_frame_resource_count());

    m_frame_fence = device.create_fence();
    m_update_fence = device.create_fence();

    return true;
}

void graphics_system::shutdown() {}

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
    auto& context = render_context::instance();

    if (context.update())
    {
        rhi_command* update_command = device.allocate_command();

        context.record(update_command);

        update_command->signal(m_update_fence.get(), ++m_update_fence_value);
        device.execute(update_command);
    }

    render();

    device.end_frame();
}

void graphics_system::render()
{
    auto& world = get_world();

    std::vector<std::tuple<const camera*, rhi_parameter*, scene*>> render_queue;

    world.get_view().read<camera>().read<camera_render_data>().read<scene_layer>().each(
        [&render_queue](
            const camera& camera,
            const camera_render_data& render_data,
            const scene_layer& layer)
        {
            if (layer.scene != nullptr)
            {
                render_queue.push_back({&camera, render_data.parameter.get(), layer.scene});
            }
        });

    std::sort(
        render_queue.begin(),
        render_queue.end(),
        [&world](auto& a, auto& b)
        {
            return std::get<0>(a)->priority > std::get<0>(b)->priority;
        });

    auto& device = render_device::instance();
    auto& context = render_context::instance();

    rhi_command* command = device.allocate_command();

    for (auto& [camera, parameter, scene] : render_queue)
    {
        rhi_fence* finish_fence = render(camera, parameter, *context.get_scene(scene->get_id()));
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
    const camera* camera,
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
} // namespace violet