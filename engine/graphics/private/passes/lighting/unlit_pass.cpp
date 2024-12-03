#include "graphics/passes/lighting/unlit_pass.hpp"

namespace violet
{
struct unlit_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/lighting/unlit.hlsl";

    struct gbuffer_data
    {
        std::uint32_t albedo;
        std::uint32_t depth;
        std::uint32_t padding0;
        std::uint32_t padding1;
    };

    static constexpr parameter gbuffer = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_FRAGMENT,
            .size = sizeof(gbuffer_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, scene},
        {2, gbuffer},
    };
};

void unlit_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rhi_parameter* bindless_parameter;
        rhi_parameter* scene_parameter;
        rhi_parameter* gbuffer_parameter;

        rdg_texture* gbuffer_albedo;
        rdg_texture* gbuffer_depth;
    };

    pass_data data = {
        .bindless_parameter = parameter.scene.get_bindless_parameter(),
        .scene_parameter = parameter.scene.get_scene_parameter(),
        .gbuffer_parameter = graph.allocate_parameter(unlit_fs::gbuffer),
        .gbuffer_albedo = parameter.gbuffer_albedo,
        .gbuffer_depth = parameter.gbuffer_depth,
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
            unlit_fs::gbuffer_data gbuffer_data = {
                .albedo = data.gbuffer_albedo->get_handle(),
                .depth = data.gbuffer_depth->get_handle(),
            };
            data.gbuffer_parameter->set_constant(0, &gbuffer_data, sizeof(unlit_fs::gbuffer_data));

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
            command.set_parameter(0, data.bindless_parameter);
            command.set_parameter(1, data.scene_parameter);
            command.set_parameter(2, data.gbuffer_parameter);
            command.draw_fullscreen();
        });
}
} // namespace violet