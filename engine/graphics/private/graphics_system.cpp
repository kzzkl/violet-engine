#include "graphics/graphics_system.hpp"
#include "components/camera.hpp"
#include "components/camera_parameter.hpp"
#include "components/light.hpp"
#include "components/mesh.hpp"
#include "components/mesh_parameter.hpp"
#include "components/skybox.hpp"
#include "components/transform.hpp"
#include "rhi_plugin.hpp"
#include "window/window_system.hpp"

namespace violet
{
graphics_system::graphics_system()
    : engine_system("Graphics")
{
}

graphics_system::~graphics_system()
{
    m_semaphores.clear();
    m_allocator = nullptr;
    m_context = nullptr;
    render_device::instance().reset();
    m_plugin->unload();
}

bool graphics_system::initialize(const dictionary& config)
{
    rhi_desc rhi_desc = {};
    rhi_desc.frame_resource_count = config["frame_resource_count"];

    m_plugin = std::make_unique<rhi_plugin>();
    m_plugin->load(config["plugin"]);
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

    task& update_transform = task_graph.get_task("Update Transform");
    task& update_component = task_graph.add_task()
                                 .set_name("Update Graphics Components")
                                 .set_group(post_update)
                                 .add_dependency(update_transform)
                                 .set_options(TASK_OPTION_MAIN_THREAD)
                                 .set_execute(
                                     [this]()
                                     {
                                         add_parameter();
                                         remove_parameter();
                                     });

    task_graph.add_task()
        .set_name("Frame End")
        .set_group(post_update)
        .add_dependency(update_component)
        .set_execute(
            [this]()
            {
                update_parameter();
                end_frame();

                m_system_version = get_world().get_version();
            });

    auto& world = get_world();
    world.register_component<light>();
    world.register_component<camera>();
    world.register_component<camera_parameter>();
    world.register_component<mesh>();
    world.register_component<mesh_parameter>();
    world.register_component<skybox>();

    m_allocator = std::make_unique<rdg_allocator>();
    m_context = std::make_unique<render_context>();

    m_used_semaphores.resize(render_device::instance().get_frame_resource_count());

    return true;
}

void graphics_system::shutdown() {}

void graphics_system::begin_frame()
{
    render_device::instance().begin_frame();
    switch_frame_resource();
    m_allocator->reset();
}

void graphics_system::end_frame()
{
    update_light();
    render();
}

void graphics_system::render()
{
    auto& world = get_world();

    std::vector<std::pair<const camera*, rhi_parameter*>> render_queue;

    world.get_view().read<camera>().read<camera_parameter>().each(
        [&render_queue](const camera& camera, const camera_parameter& parameter)
        {
            render_queue.push_back({&camera, parameter.parameter.get()});
        });

    std::sort(
        render_queue.begin(),
        render_queue.end(),
        [&world](auto& a, auto& b)
        {
            return a.first->priority > b.first->priority;
        });

    m_context->m_meshes.clear();

    world.get_view().read<mesh>().read<mesh_parameter>().each(
        [this](const mesh& mesh, const mesh_parameter& parameter)
        {
            m_context->m_meshes.push_back({&mesh, parameter.parameter.get()});
        });

    std::vector<rhi_semaphore*> finish_semaphores;
    finish_semaphores.reserve(render_queue.size());
    for (auto& [camera, parameter] : render_queue)
    {
        rhi_semaphore* finish_semaphore = render(camera, parameter);
        if (finish_semaphore)
            finish_semaphores.push_back(finish_semaphore);
    }

    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();
    device.execute(
        std::span<rhi_command*>(&command, 1),
        std::span<rhi_semaphore*>(),
        finish_semaphores,
        device.get_in_flight_fence());
    device.end_frame();
}

void graphics_system::add_parameter()
{
    auto& world = get_world();
    auto& device = render_device::instance();

    std::vector<entity> entities;

    world.get_view()
        .read<entity>()
        .read<camera>()
        .read<transform_world>()
        .without<camera_parameter>()
        .each(
            [&entities](const entity& e, const camera& camera, const transform_world& transform)
            {
                entities.push_back(e);
            });

    for (entity e : entities)
    {
        world.add_component<camera_parameter>(e);
        world.get_component<camera_parameter>(e).parameter =
            device.create_parameter(shader::camera);
    }

    entities.clear();

    world.get_view()
        .read<entity>()
        .read<mesh>()
        .read<transform_world>()
        .without<mesh_parameter>()
        .each(
            [&entities](const entity& e, const mesh& mesh, const transform_world& transform)
            {
                entities.push_back(e);
            });

    for (entity e : entities)
    {
        world.add_component<mesh_parameter>(e);
        world.get_component<mesh_parameter>(e).parameter = device.create_parameter(shader::mesh);
    }
}

void graphics_system::remove_parameter()
{
    auto& world = get_world();

    std::vector<entity> entities;

    world.get_view().read<entity>().read<camera_parameter>().without<camera>().each(
        [&entities](const entity& e, const camera_parameter& parameter)
        {
            entities.push_back(e);
        });

    for (entity e : entities)
    {
        world.remove_component<camera_parameter>(e);
    }

    entities.clear();

    world.get_view().read<entity>().read<mesh_parameter>().without<mesh>().each(
        [&entities](const entity& e, const mesh_parameter& parameter)
        {
            entities.push_back(e);
        });

    for (entity e : entities)
    {
        world.remove_component<mesh_parameter>(e);
    }
}

void graphics_system::update_parameter()
{
    auto& world = get_world();

    // Update camera.
    world.get_view().read<camera>().read<transform_world>().write<camera_parameter>().each(
        [](const camera& camera, const transform_world& transform, camera_parameter& parameter)
        {
            shader::camera_data data = {};

            matrix4 projection = math::load(camera.projection);
            matrix4 view = matrix::inverse(math::load(transform.matrix));
            matrix4 view_projection = matrix::mul(view, projection);

            data.projection = camera.projection;
            math::store(view, data.view);
            math::store(view_projection, data.view_projection);

            data.position = transform.get_position();

            parameter.parameter->set_uniform(0, &data, sizeof(shader::camera_data), 0);
        },
        [this](auto& view)
        {
            return view.is_updated<camera>(m_system_version) ||
                   view.is_updated<transform_world>(m_system_version);
        });

    world.get_view().read<skybox>().write<camera_parameter>().each(
        [](const skybox& skybox, camera_parameter& parameter)
        {
            parameter.parameter->set_texture(1, skybox.texture, skybox.sampler);
        },
        [this](auto& view)
        {
            return view.is_updated<skybox>(m_system_version);
        });

    // Update mesh.
    world.get_view().read<transform_world>().write<mesh_parameter>().each(
        [](const transform_world& transform, mesh_parameter& parameter)
        {
            shader::mesh_data data = {};
            data.model_matrix = transform.matrix;

            matrix4 normal_matrix = math::load(transform.matrix);
            normal_matrix = matrix::inverse(normal_matrix);
            normal_matrix = matrix::transpose(normal_matrix);
            math::store(normal_matrix, data.normal_matrix);

            parameter.parameter->set_uniform(0, &data, sizeof(shader::mesh_data));
        },
        [this](auto& view)
        {
            return view.is_updated<transform_world>(m_system_version);
        });
}

void graphics_system::update_light()
{
    struct directional_light
    {
        float3 direction;
        bool shadow;
        float3 color;
        std::uint32_t padding;
    };

    struct light_data
    {
        directional_light directional_lights[16];
        std::uint32_t directional_light_count;

        uint3 padding;
    };

    light_data data = {};

    get_world().get_view().write<light>().read<transform_world>().each(
        [&data](light& light, const transform_world& transform)
        {
            if (light.type == LIGHT_DIRECTIONAL)
            {
                vector4 direction = vector::set(0.0f, 0.0f, 1.0f, 0.0f);
                matrix4 world_matrix = math::load(transform.matrix);
                direction = matrix::mul(direction, world_matrix);

                math::store(
                    direction,
                    data.directional_lights[data.directional_light_count].direction);
                data.directional_lights[data.directional_light_count].color = light.color;

                ++data.directional_light_count;
            }
        });

    m_context->m_light->set_uniform(0, &data, sizeof(light_data), 0);
}

rhi_semaphore* graphics_system::render(const camera* camera, rhi_parameter* camera_parameter)
{
    std::vector<rhi_swapchain*> swapchains;
    std::vector<rhi_texture*> render_targets;

    std::vector<rhi_semaphore*> wait_semaphores;

    for (auto& render_target : camera->render_targets)
    {
        bool skip = false;

        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, rhi_swapchain*>)
                {
                    rhi_semaphore* semaphore = arg->acquire_texture();
                    if (semaphore == nullptr)
                    {
                        skip = true;
                        return;
                    }

                    wait_semaphores.push_back(semaphore);

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

    std::vector<rhi_semaphore*> signal_semaphores(swapchains.size() + 1);
    for (std::size_t i = 0; i < signal_semaphores.size(); ++i)
    {
        signal_semaphores[i] = allocate_semaphore();
    }

    render_graph graph(m_allocator.get());
    render_camera camera_data = {};
    camera_data.parameter = camera_parameter;
    camera_data.render_targets = render_targets;
    camera_data.viewport = camera->viewport;

    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();
    camera->renderer->render(graph, *m_context, camera_data);

    graph.compile();
    graph.execute(command);

    device
        .execute(std::span<rhi_command*>(&command, 1), signal_semaphores, wait_semaphores, nullptr);

    for (std::size_t i = 0; i < signal_semaphores.size() - 1; ++i)
    {
        swapchains[i]->present(&signal_semaphores[i], 1);
    }

    return signal_semaphores.back();
}

void graphics_system::switch_frame_resource()
{
    auto& device = render_device::instance();
    auto& finish_semaphores = m_used_semaphores[device.get_frame_resource_index()];
    for (rhi_semaphore* samphore : finish_semaphores)
        m_free_semaphores.push_back(samphore);
    finish_semaphores.clear();
}

rhi_semaphore* graphics_system::allocate_semaphore()
{
    auto& device = render_device::instance();

    if (m_free_semaphores.empty())
    {
        m_semaphores.emplace_back(device.create_semaphore());
        m_free_semaphores.push_back(m_semaphores.back().get());
    }

    rhi_semaphore* semaphore = m_free_semaphores.back();
    m_used_semaphores[device.get_frame_resource_index()].push_back(semaphore);

    m_free_semaphores.pop_back();

    return semaphore;
}
} // namespace violet