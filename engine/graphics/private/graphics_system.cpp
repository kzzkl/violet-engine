#include "graphics/graphics_system.hpp"
#include "components/mesh.hpp"
#include "components/transform.hpp"
#include "core/engine.hpp"
#include "rhi_plugin.hpp"
#include "window/window_system.hpp"

namespace violet
{
graphics_system::graphics_system() : engine_system("graphics"), m_idle(false)
{
}

graphics_system::~graphics_system()
{
}

bool graphics_system::initialize(const dictionary& config)
{
    auto& window = engine::get_system<window_system>();
    rect<std::uint32_t> extent = window.get_extent();

    rhi_desc rhi_desc = {};
    rhi_desc.width = extent.width;
    rhi_desc.height = extent.height;
    rhi_desc.window_handle = window.get_handle();
    rhi_desc.render_concurrency = config["render_concurrency"];
    rhi_desc.frame_resource_count = config["frame_resource_count"];
    rhi_desc.frame_resource_count = 2;

    m_plugin = std::make_unique<rhi_plugin>();
    m_plugin->load(config["plugin"]);
    m_plugin->get_rhi()->initialize(rhi_desc);

    rhi_pipeline_parameter_layout_desc node_layout_desc = {};
    node_layout_desc.parameters[0] = {RHI_PIPELINE_PARAMETER_TYPE_UNIFORM_BUFFER, sizeof(float4x4)};
    node_layout_desc.parameter_count = 1;
    add_pipeline_parameter_layout("node", node_layout_desc);

    rhi_pipeline_parameter_layout_desc texture_layout_desc = {};
    texture_layout_desc.parameters[0] = {RHI_PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE, 1};
    texture_layout_desc.parameter_count = 1;
    add_pipeline_parameter_layout("texture", texture_layout_desc);

    window.on_tick().then(
        [this]()
        {
            begin_frame();
        });
    window.on_resize().then(
        [this](std::uint32_t width, std::uint32_t height)
        {
            get_rhi()->resize(width, height);
        });
    engine::on_frame_end().then(
        [this]()
        {
            end_frame();
        });

    return true;
}

void graphics_system::render(render_graph* graph)
{
    m_render_graphs.push_back(graph);
}

void graphics_system::add_pipeline_parameter_layout(
    std::string_view name,
    const rhi_pipeline_parameter_layout_desc& desc)
{
    if (m_pipeline_parameter_layouts.find(name.data()) != m_pipeline_parameter_layouts.end())
        return;

    m_pipeline_parameter_layouts[name.data()] = get_rhi()->create_pipeline_parameter_layout(desc);
}

rhi_context* graphics_system::get_rhi() const
{
    return m_plugin->get_rhi();
}

void graphics_system::begin_frame()
{
    if (!m_idle)
        get_rhi()->begin_frame();
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

    view<mesh, transform> mesh_view(engine::get_world());
    mesh_view.each(
        [](mesh& mesh, transform& transform)
        {
            // if (transform.get_update_count() != 0)
            mesh.set_m(transform.get_world_matrix());

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

    get_rhi()->present(render_finished_semaphores.data(), render_finished_semaphores.size());
    get_rhi()->end_frame();

    return;

    /*view<mesh, transform> mesh_view(engine::get_world());

    struct render_unit
    {
        rhi_render_pipeline* pipeline;
        rhi_pipeline_parameter* material_parameter;

        rhi_resource* const* vertex_attributes;
        std::size_t vertex_attribute_count;
        std::size_t vertex_attribute_hash;

        std::size_t index_begin;
        std::size_t index_end;
        std::size_t vertex_offset;
    };

    std::vector<render_unit> render_queue;
    mesh_view.each(
        [&render_queue](mesh& mesh, transform& transform)
        {
            if (transform.get_update_count() != 0)
                mesh.get_node_parameter()->set_world_matrix(transform.get_world_matrix());

            mesh.each_render_unit(
                [&render_queue](
                    const submesh& submesh,
                    rhi_render_pipeline* pipeline,
                    rhi_pipeline_parameter* material_parameter,
                    const std::vector<rhi_resource*>& vertex_attributes,
                    std::size_t vertex_attribute_hash)
                {
                    render_queue.push_back(render_unit{
                        .pipeline = pipeline,
                        .material_parameter = material_parameter,
                        .vertex_attributes = vertex_attributes.data(),
                        .vertex_attribute_count = vertex_attributes.size(),
                        .index_begin = submesh.index_begin,
                        .index_end = submesh.index_end,
                        .vertex_offset = submesh.vertex_offset});
                });
        });

    std::sort(
        render_queue.begin(),
        render_queue.end(),
        [](const render_unit& a, const render_unit& b)
        {
            if (a.pipeline != b.pipeline)
                return a.pipeline < b.pipeline;
            if (a.material_parameter != b.material_parameter)
                return a.material_parameter < b.material_parameter;

            return a.vertex_attribute_hash < b.vertex_attribute_hash;
        });*/
}
} // namespace violet