#include "graphics/passes/lighting/unlit_pass.hpp"

namespace violet
{
struct unlit_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/source/lighting/unlit.hlsl";

    static constexpr parameter_layout parameters = {
        {0, gbuffer},
    };
};

void unlit_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture* gbuffer_albedo;
        rdg_texture* gbuffer_depth;

        rhi_parameter* gbuffer_parameter;
        rhi_sampler* gbuffer_sampler;
    };

    pass_data data = {
        .gbuffer_albedo = parameter.gbuffer_albedo,
        .gbuffer_depth = parameter.gbuffer_depth,
        .gbuffer_parameter = graph.allocate_parameter(shader::gbuffer),
        .gbuffer_sampler = graph.allocate_sampler({}),
    };

    auto& pass = graph.add_pass<rdg_render_pass>("Unlit Pass");
    pass.add_texture(
        parameter.gbuffer_albedo,
        RHI_PIPELINE_STAGE_FRAGMENT,
        RHI_ACCESS_SHADER_READ,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    pass.add_render_target(
        parameter.render_target,
        parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
    pass.set_execute(
        [data](rdg_command& command)
        {
            data.gbuffer_parameter->set_texture(
                0,
                data.gbuffer_albedo->get_rhi(),
                data.gbuffer_sampler);
            data.gbuffer_parameter->set_texture(
                1,
                data.gbuffer_depth->get_rhi(),
                data.gbuffer_sampler);

            rdg_render_pipeline pipeline = {};
            pipeline.vertex_shader = render_device::instance().get_shader<fullscreen_vs>();
            pipeline.fragment_shader = render_device::instance().get_shader<unlit_fs>();
            pipeline.depth_stencil.stencil_enable = true;
            pipeline.depth_stencil.stencil_front = {
                .compare_op = RHI_COMPARE_OP_EQUAL,
                .reference = LIGHTING_UNLIT,
            };
            pipeline.depth_stencil.stencil_back = pipeline.depth_stencil.stencil_front;

            command.set_pipeline(pipeline);
            command.set_parameter(0, data.gbuffer_parameter);
            command.draw_fullscreen();
        });
}
} // namespace violet