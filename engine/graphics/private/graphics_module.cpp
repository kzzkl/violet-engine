#include "graphics/graphics_module.hpp"
#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/mesh.hpp"
#include "components/transform.hpp"
#include "rhi_plugin.hpp"
#include "window/window_module.hpp"

namespace violet
{
class camera_component_info : public component_info_default<camera>
{
public:
    camera_component_info(render_device* device) : m_device(device) {}

    virtual void construct(actor* owner, void* target) override { new (target) camera(m_device); }

private:
    render_device* m_device;
};

class mesh_component_info : public component_info_default<mesh>
{
public:
    mesh_component_info(render_device* device) : m_device(device) {}

    virtual void construct(actor* owner, void* target) override { new (target) mesh(m_device); }

private:
    render_device* m_device;
};

graphics_module::graphics_module() : engine_module("graphics")
{
}

graphics_module::~graphics_module()
{
    m_light = nullptr;
    m_semaphores.clear();

    m_device = nullptr;
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

    m_device = std::make_unique<render_device>(m_plugin->get_rhi());

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

    get_world().register_component<mesh, mesh_component_info>(m_device.get());
    get_world().register_component<camera, camera_component_info>(m_device.get());
    get_world().register_component<light>();

    m_light = m_device->create_parameter(engine_parameter_layout::light);

    m_used_semaphores.resize(m_device->get_frame_resource_count());

    return true;
}

void graphics_module::shutdown()
{
}

void graphics_module::begin_frame()
{
    m_device->begin_frame();
    switch_frame_resource();
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

    view<mesh, transform> mesh_view(get_world());
    mesh_view.each(
        [](mesh& mesh, transform& transform)
        {
            // if (transform.get_update_count() != 0)
            mesh.set_model_matrix(transform.get_world_matrix());
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
        rhi_semaphore* finish_semaphore = allocate_semaphore();
        render_camera(camera, finish_semaphore);
        finish_semaphores.push_back(finish_semaphore);
    }

    rhi_command* command = m_device->allocate_command();
    m_device->execute({command}, {}, finish_semaphores, m_device->get_in_flight_fence());

    m_device->end_frame();
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
            if (light.type == LIGHT_TYPE_DIRECTIONAL)
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
    m_light->set_uniform(0, &data, sizeof(light_data), 0);
}

void graphics_module::render_camera(camera* camera, rhi_semaphore* finish_semaphore)
{
    view<mesh, transform> mesh_view(get_world());
    mesh_view.each(
        [this, camera](mesh& mesh, transform& transform)
        {
            // if (transform.get_update_count() != 0)
            mesh.set_model_matrix(transform.get_world_matrix());
            for (auto& submesh : mesh.get_submeshes())
            {
                render_graph* render_graph = submesh.material->get_layout()->get_render_graph();
                if (camera->get_render_graph() != render_graph)
                    continue;

                auto& passes = submesh.material->get_layout()->get_passes();
                for (std::size_t i = 0; i < passes.size(); ++i)
                {
                    rdg_mesh rdg_mesh = {
                        .material = submesh.material->get_parameter(i),
                        .transform = mesh.get_mesh_parameter(),
                        .vertex_start = submesh.vertex_start,
                        .vertex_count = submesh.vertex_count,
                        .index_start = submesh.index_start,
                        .index_count = submesh.index_count,
                        .vertex_buffers = submesh.vertex_buffers[i],
                        .index_buffer = submesh.index_buffer};
                    camera->get_render_context()->add_mesh(passes[i], rdg_mesh);
                }
            }
        });

    rdg_context* context = camera->get_render_context();
    context->set_light(m_light.get());

    std::vector<rhi_semaphore*> signal_semaphores;
    signal_semaphores.reserve(camera->get_swapchains().size() + 1);

    std::vector<rhi_semaphore*> wait_semaphores;
    for (auto& [swapchain, index] : camera->get_swapchains())
    {
        wait_semaphores.push_back(swapchain->acquire_texture());
        context->set_texture(index, swapchain->get_texture());
        signal_semaphores.push_back(allocate_semaphore());
    }
    signal_semaphores.push_back(finish_semaphore);

    rhi_command* command = m_device->allocate_command();
    camera->get_render_graph()->execute(command, context);

    m_device->execute({command}, signal_semaphores, wait_semaphores, nullptr);

    for (std::size_t i = 0; i < signal_semaphores.size() - 1; ++i)
        camera->get_swapchains()[i].first->present(&signal_semaphores[i], 1);

    context->reset();
}

void graphics_module::switch_frame_resource()
{
    auto& finish_semaphores = m_used_semaphores[m_device->get_frame_resource_index()];
    for (rhi_semaphore* samphore : finish_semaphores)
        m_free_semaphores.push_back(samphore);
    finish_semaphores.clear();
}

rhi_semaphore* graphics_module::allocate_semaphore()
{
    if (m_free_semaphores.empty())
    {
        m_semaphores.emplace_back(m_device->create_semaphore());
        m_free_semaphores.push_back(m_semaphores.back().get());
    }

    rhi_semaphore* semaphore = m_free_semaphores.back();
    m_used_semaphores[m_device->get_frame_resource_index()].push_back(semaphore);

    m_free_semaphores.pop_back();

    return semaphore;
}
} // namespace violet