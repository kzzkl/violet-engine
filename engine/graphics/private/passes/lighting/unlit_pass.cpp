#include "graphics/passes/lighting/unlit_pass.hpp"

namespace violet
{
struct unlit_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/lighting/unlit.hlsl";

    struct gbuffer_data
    {
        std::uint32_t albedo;
        std::uint32_t padding0;
        std::uint32_t padding1;
        std::uint32_t padding2;
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_FRAGMENT,
            .size = sizeof(gbuffer_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, scene},
        {2, parameter},
    };
};

void unlit_pass::add(render_graph& graph, const render_context& context, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv gbuffer_albedo;
        rhi_parameter* gbuffer_parameter;
    };

    graph.add_pass<pass_data>(
        "Unlit Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            pass.add_render_target(
                parameter.render_target,
                parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
            pass.set_depth_stencil(parameter.depth_buffer, RHI_ATTACHMENT_LOAD_OP_LOAD);

            data.gbuffer_parameter = pass.add_parameter(unlit_fs::parameter);
            data.gbuffer_albedo =
                pass.add_texture_srv(parameter.gbuffer_albedo, RHI_PIPELINE_STAGE_FRAGMENT);
        },
        [&](const pass_data& data, rdg_command& command)
        {
            unlit_fs::gbuffer_data gbuffer_data = {
                .albedo = data.gbuffer_albedo.get_bindless(),
            };
            data.gbuffer_parameter->set_constant(0, &gbuffer_data, sizeof(unlit_fs::gbuffer_data));

            auto& device = render_device::instance();

            rdg_render_pipeline pipeline = {
                .vertex_shader = device.get_shader<fullscreen_vs>(),
                .fragment_shader = device.get_shader<unlit_fs>(),
            };
            pipeline.depth_stencil.stencil_enable = true;
            pipeline.depth_stencil.stencil_front = {
                .compare_op = RHI_COMPARE_OP_EQUAL,
                .reference = SHADING_MODEL_UNLIT,
            };
            pipeline.depth_stencil.stencil_back = pipeline.depth_stencil.stencil_front;

            command.set_pipeline(pipeline);
            command.set_parameter(0, device.get_bindless_parameter());
            command.set_parameter(1, context.get_scene_parameter());
            command.set_parameter(2, data.gbuffer_parameter);
            command.draw_fullscreen();
        });
}
} // namespace violet