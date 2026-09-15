#include "graphics/renderers/passes/forward_pass.hpp"
#include "graphics/render_scene/render_scene_mesh.hpp"
#include "graphics/renderers/passes/mesh_pass.hpp"

namespace violet
{
void forward_pass::add(render_graph& graph, const parameter& parameter)
{
    const auto& mesh_module = graph.get_context().get_module<render_scene_mesh>();

    if (!mesh_module.has_material_path(MATERIAL_PATH_FORWARD))
    {
        return;
    }

    rdg_scope scope(graph, "Forward Pass");

    rhi_attachment_load_op load_op = RHI_ATTACHMENT_LOAD_OP_LOAD;

    if (parameter.main_pass && !mesh_module.has_material_path(MATERIAL_PATH_DEFERRED) &&
        !mesh_module.has_material_path(MATERIAL_PATH_VISIBILITY))
    {
        load_op = RHI_ATTACHMENT_LOAD_OP_CLEAR;
    }

    std::vector<mesh_pass::attachment> render_targets;
    render_targets.push_back({
        .texture = parameter.render_target,
        .store_op = RHI_ATTACHMENT_STORE_OP_STORE,
        .load_op = load_op,
    });

    mesh_pass::attachment depth_buffer = {
        .texture = parameter.depth_buffer,
        .store_op = RHI_ATTACHMENT_STORE_OP_STORE,
        .load_op = load_op,
    };

    graph.add_pass<mesh_pass>({
        .draw_buffer = parameter.draw_buffer,
        .draw_count_buffer = parameter.draw_count_buffer,
        .draw_info_buffer = parameter.draw_info_buffer,
        .render_targets = render_targets,
        .depth_buffer = depth_buffer,
        .surface_type = SURFACE_TYPE_OPAQUE,
        .material_path = MATERIAL_PATH_FORWARD,
    });
}
} // namespace violet