#include "graphics/graphics_module.hpp"
#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/mesh.hpp"
#include "components/transform.hpp"
#include "rhi_plugin.hpp"
#include "window/window_module.hpp"

namespace violet
{
graphics_module::graphics_module() : engine_module("graphics")
{
}

graphics_module::~graphics_module()
{
    m_semaphores.clear();
    m_allocator = nullptr;
    m_context = nullptr;
    render_device::instance().reset();
    m_plugin->unload();
}

bool graphics_module::initialize(const dictionary& config)
{
    auto& window = get_module<window_module>();
    rect<std::uint32_t> extent = window.get_extent();

    rhi_desc rhi_desc = {};
    rhi_desc.frame_resource_count = config["frame_resource_count"];

    m_plugin = std::make_unique<rhi_plugin>();
    m_plugin->load(config["plugin"]);
    m_plugin->get_rhi()->initialize(rhi_desc);

    render_device::instance().initialize(m_plugin->get_rhi());

    on_frame_begin().then(
        [this]()
        {
            begin_frame();
        });
    on_frame_end().then(
        [this]()
        {
            end_frame();
        });

    get_world().register_component<mesh>();
    get_world().register_component<camera>();
    get_world().register_component<light>();

    m_allocator = std::make_unique<rdg_allocator>();
    m_context = std::make_unique<render_context>();

    m_used_semaphores.resize(render_device::instance().get_frame_resource_count());

    return true;
}

void graphics_module::shutdown()
{
}

void graphics_module::begin_frame()
{
    render_device::instance().begin_frame();
    switch_frame_resource();
    m_allocator->reset();
}

void graphics_module::end_frame()
{
    render();
}

void graphics_module::render()
{
    update_light();

    std::vector<camera*> render_queue;

    view<camera, transform> camera_view(get_world());
    camera_view.each(
        [this, &render_queue](camera& camera, transform& transform)
        {
            if (camera.get_state() == CAMERA_STATE_DISABLE)
                return;
            if (camera.get_state() == CAMERA_STATE_RENDER_ONCE)
                camera.set_state(CAMERA_STATE_DISABLE);

            camera.set_position(transform.get_world_position());

            matrix4 view = matrix::inverse(math::load(transform.get_world_matrix()));
            camera.set_view(math::store<float4x4>(view));
            render_queue.push_back(&camera);
        });

    m_context->m_meshes.clear();
    view<mesh, transform> mesh_view(get_world());
    mesh_view.each(
        [&](mesh& mesh, transform& transform)
        {
            // if (transform.get_update_count() != 0)
            mesh.set_model_matrix(transform.get_world_matrix());
            m_context->m_meshes.push_back(&mesh);
        });

    std::sort(
        render_queue.begin(),
        render_queue.end(),
        [](camera* a, camera* b)
        {
            return a->get_priority() > b->get_priority();
        });

    std::vector<rhi_semaphore*> finish_semaphores;
    finish_semaphores.reserve(render_queue.size());
    for (camera* camera : render_queue)
    {
        rhi_semaphore* finish_semaphore = render(camera);
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

void graphics_module::update_light()
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

    view<light, transform> light_view(get_world());
    light_view.each(
        [&, this](light& light, transform& transform)
        {
            if (light.type == LIGHT_DIRECTIONAL)
            {
                vector4 direction = vector::set(0.0f, 0.0f, 1.0f, 0.0f);
                matrix4 world_matrix = math::load(transform.get_world_matrix());
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

rhi_semaphore* graphics_module::render(camera* camera)
{
    std::vector<rhi_swapchain*> swapchains = camera->get_swapchains();

    std::vector<rhi_semaphore*> wait_semaphores;
    for (rhi_swapchain* swapchain : swapchains)
    {
        rhi_semaphore* semaphore = swapchain->acquire_texture();
        if (semaphore == nullptr)
            return nullptr;

        wait_semaphores.push_back(semaphore);
    }

    std::vector<rhi_semaphore*> signal_semaphores(swapchains.size() + 1);
    for (std::size_t i = 0; i < signal_semaphores.size(); ++i)
        signal_semaphores[i] = allocate_semaphore();

    render_graph graph(m_allocator.get());
    render_camera camera_data = {};
    camera_data.parameter = camera->get_parameter();
    camera_data.render_targets = camera->get_render_targets();

    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();
    camera->get_renderer()->render(graph, *m_context, camera_data);

    graph.compile();
    graph.execute(command);

    device
        .execute(std::span<rhi_command*>(&command, 1), signal_semaphores, wait_semaphores, nullptr);

    for (std::size_t i = 0; i < signal_semaphores.size() - 1; ++i)
        swapchains[i]->present(&signal_semaphores[i], 1);

    return signal_semaphores.back();
}

void graphics_module::switch_frame_resource()
{
    auto& device = render_device::instance();
    auto& finish_semaphores = m_used_semaphores[device.get_frame_resource_index()];
    for (rhi_semaphore* samphore : finish_semaphores)
        m_free_semaphores.push_back(samphore);
    finish_semaphores.clear();
}

rhi_semaphore* graphics_module::allocate_semaphore()
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