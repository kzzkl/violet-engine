#include "graphics/graphics_system.hpp"
#include "components/camera.hpp"
#include "components/mesh.hpp"
#include "components/transform.hpp"
#include "rhi_plugin.hpp"
#include "window/window_system.hpp"

namespace violet
{
class mesh_component_info : public component_info_default<mesh>
{
public:
    mesh_component_info(rhi_renderer* rhi, rhi_parameter_layout* mesh_parameter_layout)
        : m_rhi(rhi),
          m_mesh_parameter_layout(mesh_parameter_layout)
    {
    }

    virtual void construct(actor* owner, void* target) override
    {
        new (target) mesh(m_rhi, m_mesh_parameter_layout);
    }

private:
    rhi_renderer* m_rhi;
    rhi_parameter_layout* m_mesh_parameter_layout;
};

graphics_system::graphics_system() : engine_system("graphics"), m_idle(false)
{
}

graphics_system::~graphics_system()
{
    m_context = nullptr;
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

    m_context = std::make_unique<graphics_context>(m_plugin->get_rhi());

    window.on_tick().then(
        [this]()
        {
            begin_frame();
        });
    window.on_resize().then(
        [this](std::uint32_t width, std::uint32_t height)
        {
            m_plugin->get_rhi()->resize(width, height);
        });
    on_frame_end().then(
        [this]()
        {
            end_frame();
        });

    get_world().register_component<mesh, mesh_component_info>(
        m_plugin->get_rhi(),
        m_context->get_parameter_layout("violet mesh"));
    get_world().register_component<camera>();

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
        m_context->get_rhi()->begin_frame();
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

    view<camera, transform> camera_view(get_world());
    camera_view.each(
        [this](camera& camera, transform& transform)
        {
            camera.set_view(matrix::inverse(transform.get_world_matrix()));
            camera.set_back_buffer(m_context->get_rhi()->get_back_buffer());

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
        render_graph->execute();
        render_finished_semaphores.push_back(render_graph->get_render_finished_semaphore());
    }
    m_render_graphs.clear();

    m_context->get_rhi()->present(
        render_finished_semaphores.data(),
        render_finished_semaphores.size());
    m_context->get_rhi()->end_frame();
}
} // namespace violet