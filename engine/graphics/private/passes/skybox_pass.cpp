#include "graphics/passes/skybox_pass.hpp"

namespace violet
{
struct skybox_vs : public vertex_shader
{
    static std::string_view get_path() { return "assets/shaders/skybox.vs"; }

    static constexpr parameter_slots get_parameters()
    {
        return {
            {0, shader::camera}
        };
    };
};

struct skybox_fs : public fragment_shader
{
    static std::string_view get_path() { return "assets/shaders/skybox.fs"; }

    static constexpr parameter_slots get_parameters()
    {
        return {
            {0, shader::camera}
        };
    };
};

skybox_pass::skybox_pass(const data& data)
{
    add_render_target(data.render_target, data.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
    set_depth_stencil(data.depth_buffer, data.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
    set_execute(
        [&data](rdg_command* command)
        {
            command->set_viewport(data.viewport);

            rhi_scissor_rect scissor = {
                .min_x = 0,
                .min_y = 0,
                .max_x = static_cast<std::uint32_t>(data.viewport.width),
                .max_y = static_cast<std::uint32_t>(data.viewport.height)};
            command->set_scissor(std::span<rhi_scissor_rect>(&scissor, 1));

            rdg_render_pipeline pipeline = {};
            pipeline.vertex_shader = render_device::instance().get_shader<skybox_vs>();
            pipeline.fragment_shader = render_device::instance().get_shader<skybox_fs>();

            command->set_pipeline(pipeline);
            command->set_parameter(0, data.camera);
            command->draw(0, 36);
        });
}
} // namespace violet