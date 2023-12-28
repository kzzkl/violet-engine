#include "graphics/pipeline/skybox_pipeline.hpp"

namespace violet
{
skybox_pipeline::skybox_pipeline()
{
}

bool skybox_pipeline::compile(compile_context& context)
{
    set_shader("engine/shaders/skybox.vert.spv", "engine/shaders/skybox.frag.spv");
    set_cull_mode(RHI_CULL_MODE_BACK);

    set_parameter_layouts({
        {context.renderer->get_parameter_layout("violet camera"),
         RENDER_PIPELINE_PARAMETER_TYPE_CAMERA}
    });

    return render_pipeline::compile(context);
}

void skybox_pipeline::render(std::vector<render_mesh>& meshes, const execute_context& context)
{
    context.command->set_render_parameter(0, context.camera);
    context.command->draw(0, 36);
}
} // namespace violet