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

void skybox_pass::add(
    render_graph& graph,
    const render_context& context,
    const parameter& parameter)
{
    struct pass_data
    {
    };

    graph.add_pass<pass_data>(
        "Skybox Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            rhi_attachment_load_op load_op =
                parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD;

            pass.add_render_target(parameter.render_target, load_op);
            pass.set_depth_stencil(parameter.depth_buffer, load_op);
        },
        [&](const pass_data& data, rdg_command& command)
        {
            const auto& viewport = context.get_viewport();

            command.set_viewport(viewport);

            rhi_scissor_rect scissor = {
                .min_x = 0,
                .min_y = 0,
                .max_x = static_cast<std::uint32_t>(viewport.width),
                .max_y = static_cast<std::uint32_t>(viewport.height),
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
            command.set_parameter(1, context.get_scene_parameter());
            command.set_parameter(2, context.get_camera_parameter());
            command.draw(0, 36);
        });
}
} // namespace violet