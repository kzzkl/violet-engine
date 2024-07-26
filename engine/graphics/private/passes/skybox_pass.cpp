#include "graphics/passes/skybox_pass.hpp"

namespace violet
{
struct skybox_vs : public vertex_shader<skybox_vs>
{
    static constexpr std::string_view path = "engine/shaders/skybox.vs";
    static constexpr parameter_slot parameters[] = {
        {0, shader::camera}
    };
};

struct skybox_fs : public fragment_shader<skybox_fs>
{
    static constexpr std::string_view path = "engine/shaders/skybox.fs";
    static constexpr parameter_slot parameters[] = {
        {0, shader::camera}
    };
};

skybox_pass::skybox_pass(const data& data)
{
    add_render_target(data.render_target);
    set_depth_stencil(data.depth_buffer);
    set_execute(
        [&data](rdg_command* command)
        {
            rhi_texture_extent extent = data.render_target->get_rhi()->get_extent();
            rhi_viewport viewport = {
                .x = 0,
                .y = 0,
                .width = static_cast<float>(extent.width),
                .height = static_cast<float>(extent.height),
                .min_depth = 0.0f,
                .max_depth = 1.0f};
            command->set_viewport(viewport);

            rhi_scissor_rect scissor =
                {.min_x = 0, .min_y = 0, .max_x = extent.width, .max_y = extent.height};
            command->set_scissor(std::span<rhi_scissor_rect>(&scissor, 1));

            rdg_render_pipeline pipeline = {};
            pipeline.vertex_shader = skybox_vs::get_rhi();
            pipeline.fragment_shader = skybox_fs::get_rhi();

            command->set_pipeline(pipeline);
            command->set_parameter(0, data.camera);
            command->draw(0, 36);
        });
}
} // namespace violet