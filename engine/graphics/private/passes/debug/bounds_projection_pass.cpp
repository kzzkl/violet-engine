#include "graphics/passes/debug/bounds_projection_pass.hpp"

namespace violet
{
struct bounds_projection_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/debug/bounds_projection.hlsl";
};

struct bounds_projection_gs : public shader_gs
{
    static constexpr std::string_view path = "assets/shaders/debug/bounds_projection.hlsl";

    static constexpr parameter_layout parameters = {
        {0, shader::bindless},
        {1, shader::scene},
        {2, shader::camera},
    };
};

struct bounds_projection_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/debug/bounds_projection.hlsl";
};

void bounds_projection_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        std::uint32_t instance_count;
    };

    graph.add_pass<pass_data>(
        "Bounds Projection Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            pass.add_render_target(parameter.render_target, RHI_ATTACHMENT_LOAD_OP_LOAD);
            data.instance_count = graph.get_scene().get_instance_count();
        },
        [](const pass_data& data, rdg_command& command)
        {
            command.set_viewport();
            command.set_scissor();

            auto& device = render_device::instance();
            rdg_raster_pipeline pipeline = {
                .vertex_shader = device.get_shader<bounds_projection_vs>(),
                .geometry_shader = device.get_shader<bounds_projection_gs>(),
                .fragment_shader = device.get_shader<bounds_projection_fs>(),
                .primitive_topology = RHI_PRIMITIVE_TOPOLOGY_POINT_LIST,
            };

            command.set_pipeline(pipeline);
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);
            command.draw(0, data.instance_count);
        });
}
} // namespace violet