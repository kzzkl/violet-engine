#include "mmd_render.hpp"

namespace violet::sample
{
class color_pipeline : public render_pipeline
{
public:
    color_pipeline(graphics_context* context) : render_pipeline(context)
    {
        set_shader("mmd-viewer/shaders/color.vert.spv", "mmd-viewer/shaders/color.frag.spv");
        set_vertex_attributes({
            {"position", RHI_RESOURCE_FORMAT_R32G32B32_FLOAT},
            {"normal",   RHI_RESOURCE_FORMAT_R32G32B32_FLOAT},
            {"uv",       RHI_RESOURCE_FORMAT_R32G32_FLOAT   }
        });
        set_cull_mode(RHI_CULL_MODE_NONE);

        rhi_parameter_layout* material_layout = context->add_parameter_layout(
            "mmd material",
            {
                {RHI_PARAMETER_TYPE_UNIFORM_BUFFER,  sizeof(mmd_material), false, true}, // Uniform
                {RHI_PARAMETER_TYPE_SHADER_RESOURCE, 1,                    false, true}, // Tex
                {RHI_PARAMETER_TYPE_SHADER_RESOURCE, 1,                    false, true}, // Toon
                {RHI_PARAMETER_TYPE_SHADER_RESOURCE, 1,                    false, true}  // Spa
        });

        set_parameter_layouts({
            {context->get_parameter_layout("violet mesh"),   RENDER_PIPELINE_PARAMETER_TYPE_MESH    },
            {material_layout,                                RENDER_PIPELINE_PARAMETER_TYPE_MATERIAL},
            {context->get_parameter_layout("violet camera"),
             RENDER_PIPELINE_PARAMETER_TYPE_CAMERA                                                  }
        });
    }

private:
    void render(rhi_render_command* command, render_data& data)
    {
        for (render_mesh& mesh : data.meshes)
        {
            command->set_vertex_buffers(mesh.vertex_buffers.data(), mesh.vertex_buffers.size());
            command->set_index_buffer(mesh.index_buffer);
            command->set_parameter(0, mesh.node);
            command->set_parameter(1, mesh.material);
            command->set_parameter(2, data.camera_parameter);
            command->draw_indexed(mesh.index_start, mesh.index_count, mesh.vertex_start);
        }
    }
};

mmd_render_graph::mmd_render_graph(graphics_context* context) : render_graph(context)
{
    render_pass* main_pass = add_render_pass("main");

    render_attachment* output_attachment = main_pass->add_attachment("output");
    output_attachment->set_format(context->get_rhi()->get_back_buffer()->get_format());
    output_attachment->set_initial_state(RHI_RESOURCE_STATE_UNDEFINED);
    output_attachment->set_final_state(RHI_RESOURCE_STATE_PRESENT);
    output_attachment->set_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
    output_attachment->set_store_op(RHI_ATTACHMENT_STORE_OP_STORE);

    render_attachment* depth_attachment = main_pass->add_attachment("depth");
    depth_attachment->set_format(RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT);
    depth_attachment->set_initial_state(RHI_RESOURCE_STATE_UNDEFINED);
    depth_attachment->set_final_state(RHI_RESOURCE_STATE_DEPTH_STENCIL);
    depth_attachment->set_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
    depth_attachment->set_store_op(RHI_ATTACHMENT_STORE_OP_DONT_CARE);
    depth_attachment->set_stencil_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
    depth_attachment->set_stencil_store_op(RHI_ATTACHMENT_STORE_OP_DONT_CARE);

    render_subpass* color_pass = main_pass->add_subpass("color");
    color_pass->add_reference(
        output_attachment,
        RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
        RHI_RESOURCE_STATE_RENDER_TARGET);
    color_pass->add_reference(
        depth_attachment,
        RHI_ATTACHMENT_REFERENCE_TYPE_DEPTH_STENCIL,
        RHI_RESOURCE_STATE_DEPTH_STENCIL);

    render_pipeline* pipeline = color_pass->add_pipeline<color_pipeline>("color pipeline");
    material_layout* material_layout = add_material_layout("mmd material");
    material_layout->add_pipeline(pipeline);
    material_layout->add_field("mmd material", {0, 0, sizeof(mmd_material), 0});
    material_layout->add_field("mmd tex", {0, 1, 1, 0});
    material_layout->add_field("mmd toon", {0, 2, 1, 0});
    material_layout->add_field("mmd spa", {0, 3, 1, 0});
}
} // namespace violet::sample