#include "graphics/renderers/passes/depth_only_pass.hpp"
#include "graphics/renderers/passes/mesh_pass.hpp"

namespace violet
{
struct depth_only_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/depth_only.hlsl";
};

void depth_only_pass::add(render_graph& graph, const parameter& parameter)
{
    auto& device = render_device::instance();

    rdg_raster_pipeline pipeline = {
        .vertex_shader = device.get_shader<depth_only_vs>(),
        .rasterizer_state = device.get_rasterizer_state(parameter.cull_mode),
        .depth_stencil_state = device.get_depth_stencil_state(
            true,
            true,
            parameter.depth_compare_op,
            parameter.stencil_enable,
            parameter.stencil_front,
            parameter.stencil_back),
        .primitive_topology = parameter.primitive_topology,
    };

    mesh_pass::attachment depth_buffer = {
        .texture = parameter.depth_buffer,
        .store_op = RHI_ATTACHMENT_STORE_OP_STORE,
        .load_op = parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD,
    };

    graph.add_pass<mesh_pass>({
        .draw_buffer = parameter.draw_buffer,
        .draw_count_buffer = parameter.draw_count_buffer,
        .draw_info_buffer = parameter.draw_info_buffer,
        .depth_buffer = depth_buffer,
        .surface_type = parameter.surface_type,
        .override_pipeline = pipeline,
    });
}
} // namespace violet