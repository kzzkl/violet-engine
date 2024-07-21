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

void skybox_pass::render(
    render_graph& graph,
    const render_camera& camera,
    rdg_texture* render_target,
    rdg_texture* depth_buffer)
{
    struct pass_data : public rdg_data
    {
        rdg_texture* render_target;
        rhi_parameter* camera;
    };

    pass_data& data = graph.allocate_data<pass_data>();
    data.render_target = render_target;
    data.camera = camera.parameter;

    auto pass = graph.add_pass<rdg_render_pass>("Skybox Pass");
    pass->add_render_target(render_target);
    pass->set_depth_stencil(depth_buffer);
    pass->set_execute(
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