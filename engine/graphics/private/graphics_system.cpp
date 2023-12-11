#include "graphics/graphics_system.hpp"
#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/mesh.hpp"
#include "components/skybox.hpp"
#include "components/transform.hpp"
#include "rhi_plugin.hpp"
#include "window/window_system.hpp"

namespace violet
{
class camera_component_info : public component_info_default<camera>
{
public:
    camera_component_info(renderer* renderer) : m_renderer(renderer) {}

    virtual void construct(actor* owner, void* target) override { new (target) camera(m_renderer); }

private:
    renderer* m_renderer;
};

class mesh_component_info : public component_info_default<mesh>
{
public:
    mesh_component_info(renderer* renderer) : m_renderer(renderer) {}

    virtual void construct(actor* owner, void* target) override { new (target) mesh(m_renderer); }

private:
    renderer* m_renderer;
};

graphics_system::graphics_system() : engine_system("graphics"), m_idle(false)
{
}

graphics_system::~graphics_system()
{
    m_renderer = nullptr;
    m_plugin->unload();
}

bool graphics_system::initialize(const dictionary& config)
{
    auto& window = get_system<window_system>();
    rect<std::uint32_t> extent = window.get_extent();

    rhi_desc rhi_desc = {};
    rhi_desc.width = extent.width;
    rhi_desc.height = extent.height;
    rhi_desc.window_handle = window.get_handle();
    rhi_desc.render_concurrency = config["render_concurrency"];
    rhi_desc.frame_resource_count = config["frame_resource_count"];

    m_plugin = std::make_unique<rhi_plugin>();
    m_plugin->load(config["plugin"]);
    m_plugin->get_rhi()->initialize(rhi_desc);

    m_renderer = std::make_unique<renderer>(m_plugin->get_rhi());

    window.on_tick().then(
        [this]()
        {
            begin_frame();
        });
    window.on_resize().then(
        [this](std::uint32_t width, std::uint32_t height)
        {
            m_renderer->resize(width, height);
        });
    on_frame_end().then(
        [this]()
        {
            end_frame();
        });

    get_world().register_component<mesh, mesh_component_info>(m_renderer.get());
    get_world().register_component<camera, camera_component_info>(m_renderer.get());
    get_world().register_component<light>();
    get_world().register_component<skybox>();

    return true;
}

void graphics_system::shutdown()
{
}

void graphics_system::render(render_graph* graph)
{
    m_render_graphs.push_back(graph);
}

void graphics_system::begin_frame()
{
    if (!m_idle)
        m_renderer->begin_frame();
}

void graphics_system::end_frame()
{
    render();
}

void graphics_system::render()
{
    m_idle = m_render_graphs.empty();
    if (m_idle)
        return;

    update_light();

    view<camera, transform> camera_view(get_world());
    camera_view.each(
        [this](camera& camera, transform& transform)
        {
            camera.set_position(transform.get_world_position());
            camera.set_view(matrix::inverse(transform.get_world_matrix()));
            camera.set_back_buffer(m_renderer->get_back_buffer());

            render_pass* render_pass = camera.get_render_pass();
            render_pass->add_camera(
                camera.get_scissor(),
                camera.get_viewport(),
                camera.get_parameter(),
                camera.get_framebuffer());
        });

    view<mesh, transform> mesh_view(get_world());
    mesh_view.each(
        [](mesh& mesh, transform& transform)
        {
            // if (transform.get_update_count() != 0)
            mesh.set_model_matrix(transform.get_world_matrix());
            mesh.each_submesh(
                [](const render_mesh& submesh, render_pipeline* pipeline)
                {
                    pipeline->add_mesh(submesh);
                });
        });

    std::vector<rhi_semaphore*> render_finished_semaphores;
    render_finished_semaphores.reserve(m_render_graphs.size());
    for (render_graph* render_graph : m_render_graphs)
    {
        render_graph->execute(m_renderer->get_light_parameter());
        render_finished_semaphores.push_back(render_graph->get_render_finished_semaphore());
    }
    m_render_graphs.clear();

    m_renderer->present(render_finished_semaphores);
    m_renderer->end_frame();
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

    rhi_parameter* light_parameter = m_renderer->get_light_parameter();

    view<light, transform> light_view(get_world());
    light_view.each(
        [&, this](light& light, transform& transform)
        {
            if (light.type == LIGHT_TYPE_DIRECTIONAL)
            {
                float4_simd direction = simd::set(1.0f, 0.0f, 0.0f, 0.0f);
                float4x4_simd world_matrix = simd::load(transform.get_world_matrix());
                direction = matrix_simd::mul(direction, world_matrix);

                simd::store(
                    direction,
                    data.directional_lights[data.directional_light_count].direction);
                data.directional_lights[data.directional_light_count].color = light.color;

                ++data.directional_light_count;
            }
        });

    m_renderer->get_light_parameter()->set_uniform(0, &data, sizeof(light_data), 0);
}
} // namespace violet