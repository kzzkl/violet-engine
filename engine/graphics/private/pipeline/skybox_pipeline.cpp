#include "graphics/pipeline/skybox_pipeline.hpp"

namespace violet
{
skybox_pipeline::skybox_pipeline(std::string_view name, renderer* renderer)
    : render_pipeline(name, renderer)
{
    set_shader("engine/shaders/skybox.vert.spv", "engine/shaders/skybox.frag.spv");
    set_cull_mode(RHI_CULL_MODE_NONE);

    set_parameter_layouts({
        {renderer->get_parameter_layout("violet camera"), RENDER_PIPELINE_PARAMETER_TYPE_CAMERA}
    });
}

void skybox_pipeline::render(rhi_render_command* command, render_data& data)
{
    command->set_render_parameter(0, data.camera);
    command->draw(0, 36);
}
} // namespace violet