#include "pbr_render.hpp"
#include "graphics/passes/skybox_pass.hpp"

namespace violet::sample
{
class pbr_pass : public render_pass
{
public:
    pbr_pass(renderer* renderer, setup_context& context) : render_pass(renderer, context)
    {
        renderer->add_parameter_layout(
            "pbr material",
            {
                {RHI_PARAMETER_TYPE_UNIFORM_BUFFER, 32, RHI_PARAMETER_STAGE_FLAG_FRAGMENT},
                {RHI_PARAMETER_TYPE_TEXTURE,        1,  RHI_PARAMETER_STAGE_FLAG_FRAGMENT}
        });

        m_render_target = context.write("back buffer");
        m_depth_buffer = context.write("depth buffer");

        render_attachment* render_target_attachment = add_attachment(m_render_target);
        render_target_attachment->set_initial_layout(RHI_IMAGE_LAYOUT_UNDEFINED);
        render_target_attachment->set_final_layout(RHI_IMAGE_LAYOUT_RENDER_TARGET);
        render_target_attachment->set_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
        render_target_attachment->set_store_op(RHI_ATTACHMENT_STORE_OP_STORE);

        render_attachment* depth_buffer_attachment = add_attachment(m_depth_buffer);
        depth_buffer_attachment->set_initial_layout(RHI_IMAGE_LAYOUT_UNDEFINED);
        depth_buffer_attachment->set_final_layout(RHI_IMAGE_LAYOUT_DEPTH_STENCIL);
        depth_buffer_attachment->set_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
        depth_buffer_attachment->set_store_op(RHI_ATTACHMENT_STORE_OP_STORE);
        depth_buffer_attachment->set_stencil_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
        depth_buffer_attachment->set_stencil_store_op(RHI_ATTACHMENT_STORE_OP_STORE);

        render_subpass* subpass = add_subpass();
        subpass->add_reference(
            render_target_attachment,
            RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
            RHI_IMAGE_LAYOUT_RENDER_TARGET);
        subpass->add_reference(
            depth_buffer_attachment,
            RHI_ATTACHMENT_REFERENCE_TYPE_DEPTH_STENCIL,
            RHI_IMAGE_LAYOUT_DEPTH_STENCIL);

        m_pipeline = add_pipeline(subpass);
        m_pipeline->set_shader("pbr/shaders/pbr.vert.spv", "pbr/shaders/pbr.frag.spv");
        m_pipeline->set_vertex_attributes({
            {"position", RHI_RESOURCE_FORMAT_R32G32B32_FLOAT},
            {"normal",   RHI_RESOURCE_FORMAT_R32G32B32_FLOAT}
        });
        m_pipeline->set_cull_mode(RHI_CULL_MODE_BACK);

        m_pipeline->set_parameter_layouts(
            {renderer->get_parameter_layout("violet mesh"),
             renderer->get_parameter_layout("pbr material"),
             renderer->get_parameter_layout("violet camera"),
             renderer->get_parameter_layout("violet light")});

        context.register_material_pipeline(
            "pbr",
            m_pipeline,
            renderer->get_parameter_layout("pbr material"));
    }

    virtual void execute(execute_context& context) override
    {
        rhi_render_command* command = context.get_command();

        rhi_resource_extent extent = m_render_target->get_image()->get_extent();

        rhi_scissor_rect scissor;
        scissor.min_x = 0;
        scissor.min_y = 0;
        scissor.max_x = extent.width;
        scissor.max_y = extent.height;
        command->set_scissor(&scissor, 1);

        rhi_viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.min_depth = 0.0f;
        viewport.max_depth = 1.0f;
        command->set_viewport(viewport);

        command->begin(get_interface(), get_framebuffer({m_render_target, m_depth_buffer}));

        command->set_render_pipeline(m_pipeline->get_interface());
        command->set_render_parameter(2, context.get_camera("main camera"));
        command->set_render_parameter(3, context.get_light());

        for (const render_mesh& mesh : m_pipeline->get_meshes())
        {
            command->set_vertex_buffers(mesh.vertex_buffers.data(), mesh.vertex_buffers.size());
            command->set_index_buffer(mesh.index_buffer);
            command->set_render_parameter(0, mesh.transform);
            command->set_render_parameter(1, mesh.material);
            command->draw_indexed(mesh.index_start, mesh.index_count, mesh.vertex_start);
        }

        command->end();
    }

private:
    render_resource* m_render_target;
    render_resource* m_depth_buffer;
    render_pipeline* m_pipeline;
};

void pbr_material::set_albedo(const float3& albedo)
{
    get_parameter(0)->set_uniform(0, &albedo, sizeof(float3), 0);
}

void pbr_material::set_metalness(float metalness)
{
    get_parameter(0)->set_uniform(0, &metalness, sizeof(float), 12);
}

void pbr_material::set_roughness(float roughness)
{
    get_parameter(0)->set_uniform(0, &roughness, sizeof(float), 16);
}

std::vector<std::string> pbr_material::get_layout() const
{
    return {"pbr"};
}

pbr_render_graph::pbr_render_graph(renderer* renderer) : render_graph(renderer)
{
    render_resource* depth_buffer = add_resource("depth buffer");
    depth_buffer->set_format(RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT);
    depth_buffer->set_samples(RHI_SAMPLE_COUNT_1);

    add_pass<pbr_pass>("pbr");
    add_pass<skybox_pass>("skybox");

    compile();
}
} // namespace violet::sample