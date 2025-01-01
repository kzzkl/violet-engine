#include "graphics/passes/skybox_pass.hpp"
#include "graphics/render_interface.hpp"
#include "graphics/shader.hpp"

namespace violet
{
struct skybox_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/skybox.hlsl";

    static constexpr parameter_layout parameters = {
        {0, shader::bindless},
        {2, shader::camera},
    };
};

struct skybox_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/skybox.hlsl";

    static constexpr parameter_layout parameters = {
        {0, shader::bindless},
        {1, shader::scene},
    };
};

void skybox_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rhi_parameter* scene_parameter;
        rhi_parameter* camera_parameter;
        rhi_viewport viewport;
    };

    pass_data data = {
        .scene_parameter = parameter.scene.get_scene_parameter(),
        .camera_parameter = parameter.camera.camera_parameter,
        .viewport = parameter.camera.viewport,
    };

    auto& pass = graph.add_pass<rdg_render_pass>("Skybox Pass");

    pass.add_render_target(
        parameter.render_target,
        parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
    pass.set_depth_stencil(
        parameter.depth_buffer,
        parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
    pass.set_execute(
        [data](rdg_command& command)
        {
            command.set_viewport(data.viewport);

            rhi_scissor_rect scissor = {
                .min_x = 0,
                .min_y = 0,
                .max_x = static_cast<std::uint32_t>(data.viewport.width),
                .max_y = static_cast<std::uint32_t>(data.viewport.height),
            };
            command.set_scissor(std::span<rhi_scissor_rect>(&scissor, 1));

            auto& device = render_device::instance();

            rdg_render_pipeline pipeline = {
                .vertex_shader = device.get_shader<skybox_vs>(),
                .fragment_shader = device.get_shader<skybox_fs>(),
                .depth_stencil =
                    {
                        .depth_enable = true,
                        .depth_compare_op = RHI_COMPARE_OP_GREATER,
                    },
            };

            command.set_pipeline(pipeline);
            command.set_parameter(0, device.get_bindless_parameter());
            command.set_parameter(1, data.scene_parameter);
            command.set_parameter(2, data.camera_parameter);
            command.draw(0, 36);
        });
}
} // namespace violet