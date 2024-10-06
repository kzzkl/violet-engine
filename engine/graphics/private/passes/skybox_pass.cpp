#include "graphics/passes/skybox_pass.hpp"

namespace violet
{
struct skybox_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/source/skybox.hlsl";

    static constexpr parameter_layout parameters = {{0, shader::camera}};
};

struct skybox_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/source/skybox.hlsl";

    static constexpr parameter_layout parameters = {{0, shader::camera}};
};

void skybox_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data : public rdg_data
    {
        rhi_parameter* camera;
        rhi_viewport viewport;
    };

    pass_data& data = graph.allocate_data<pass_data>();
    data.viewport = parameter.viewport;
    data.camera = parameter.camera;

    rdg_render_pass* pass = graph.add_pass<rdg_render_pass>("Skybox Pass");

    pass->add_render_target(
        parameter.render_target,
        parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
    pass->set_depth_stencil(
        parameter.depth_buffer,
        parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
    pass->set_execute(
        [data](rdg_command& command)
        {
            command.set_viewport(data.viewport);

            rhi_scissor_rect scissor = {
                .min_x = 0,
                .min_y = 0,
                .max_x = static_cast<std::uint32_t>(data.viewport.width),
                .max_y = static_cast<std::uint32_t>(data.viewport.height)};
            command.set_scissor(std::span<rhi_scissor_rect>(&scissor, 1));

            rdg_render_pipeline pipeline = {};
            pipeline.vertex_shader = render_device::instance().get_shader<skybox_vs>();
            pipeline.fragment_shader = render_device::instance().get_shader<skybox_fs>();

            command.set_pipeline(pipeline);
            command.set_parameter(0, data.camera);
            command.draw(0, 36);
        });
}
} // namespace violet