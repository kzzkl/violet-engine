#include "graphics/graphics_module.hpp"
#include "components/mesh.hpp"
#include "components/transform.hpp"
#include "core/context/engine.hpp"
#include "rhi_plugin.hpp"
#include "window/window_module.hpp"
#include <queue>
#include <unordered_set>

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

    engine::get_task_graph().end_frame.add_task("present", [this]() { get_rhi()->present(); });

    return true;
}

void graphics_module::render()
{
    view<mesh, transform> mesh_view(engine::get_world());

    std::unordered_set<render_pipeline*> render_pipelines;
    mesh_view.each([&render_pipelines](mesh& mesh, transform& transform) {
        if (transform.get_update_count() != 0)
            mesh.get_node_parameter()->set_world_matrix(transform.get_world_matrix());

        mesh.each_submesh([&mesh, &render_pipelines](
                              const submesh& submesh,
                              const material& material,
                              const std::vector<rhi_resource*>& vertex_buffers,
                              rhi_resource* index_buffer) {
            render_item item = {
                .index_buffer = index_buffer,
                .vertex_buffers = vertex_buffers.data(),
                .node_parameter = mesh.get_node_parameter()->get_interface(),
                .material_parameter = material.parameter};
            //material.pipeline->add_item(item);

            render_pipelines.insert(material.pipeline);
        });
    });
}

rhi_context* graphics_module::get_rhi() const
{
    return m_plugin->get_rhi();
}
} // namespace violet