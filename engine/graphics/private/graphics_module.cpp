#include "graphics/graphics_module.hpp"
#include "components/mesh.hpp"
#include "components/transform.hpp"
#include "core/context/engine.hpp"
#include "graphics/graphics_task.hpp"
#include "rhi_plugin.hpp"
#include "window/window_module.hpp"

namespace violet
{
graphics_module::graphics_module() : engine_module("graphics")
{
}

graphics_module::~graphics_module()
{
}

bool graphics_module::initialize(const dictionary& config)
{
    auto& window = engine::get_module<window_module>();
    rect<std::uint32_t> extent = window.get_extent();

    rhi_desc rhi_desc = {};
    rhi_desc.width = extent.width;
    rhi_desc.height = extent.height;
    rhi_desc.window_handle = window.get_handle();
    rhi_desc.render_concurrency = config["render_concurrency"];
    rhi_desc.frame_resource = config["frame_resource"];

    m_plugin = std::make_unique<rhi_plugin>();
    m_plugin->load(config["plugin"]);
    m_plugin->get_rhi()->initialize(rhi_desc);

    m_render_graph = std::make_unique<render_graph>(m_plugin->get_rhi());

    auto& begin_frame_graph = engine::get_task_graph().begin_frame;
    task* begin_frame_task = begin_frame_graph.add_task(
        TASK_NAME_GRAPHICS_BEGIN_FRAME,
        [this]() { get_rhi()->begin_frame(); });
    begin_frame_graph.add_dependency(
        begin_frame_graph.get_task(TASK_NAME_WINDOW_TICK),
        begin_frame_task);

    auto& end_frame_graph = engine::get_task_graph().end_frame;
    task* render_task = end_frame_graph.add_task(
        TASK_NAME_GRAPHICS_RENDER,
        [this]()
        {
            render();
            get_rhi()->present();
        });
    end_frame_graph.add_dependency(end_frame_graph.get_root(), render_task);

    return true;
}

rhi_context* graphics_module::get_rhi() const
{
    return m_plugin->get_rhi();
}

void graphics_module::render()
{
    view<mesh, transform> mesh_view(engine::get_world());

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
        });
}
} // namespace violet