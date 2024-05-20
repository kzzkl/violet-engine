#include "mmd_render.hpp"
#include "graphics/passes/present_pass.hpp"
// #include "graphics/pipeline/debug_pipeline.hpp"

namespace violet::sample
{
mmd_material::mmd_material(render_device* device, material_layout* layout)
    : material(device, layout)
{
}

void mmd_material::set_diffuse(const float4& diffuse)
{
    get_parameter(0)->set_uniform(0, &diffuse, sizeof(float4), 0);
}

void mmd_material::set_specular(float3 specular, float specular_strength)
{
    get_parameter(0)->set_uniform(0, &specular, sizeof(float3), 16);
    get_parameter(0)->set_uniform(0, &specular_strength, sizeof(float), 28);
}

void mmd_material::set_ambient(const float3& ambient)
{
    get_parameter(0)->set_uniform(0, &ambient, sizeof(float3), 32);
}

void mmd_material::set_toon_mode(std::uint32_t toon_mode)
{
    get_parameter(0)->set_uniform(0, &toon_mode, sizeof(std::uint32_t), 44);
}

void mmd_material::set_spa_mode(std::uint32_t spa_mode)
{
    get_parameter(0)->set_uniform(0, &spa_mode, sizeof(std::uint32_t), 48);
}

void mmd_material::set_tex(rhi_texture* texture, rhi_sampler* sampler)
{
    get_parameter(0)->set_texture(1, texture, sampler);
}

void mmd_material::set_toon(rhi_texture* texture, rhi_sampler* sampler)
{
    get_parameter(0)->set_texture(2, texture, sampler);
}

void mmd_material::set_spa(rhi_texture* texture, rhi_sampler* sampler)
{
    get_parameter(0)->set_texture(3, texture, sampler);
}

void mmd_material::set_edge(const float4& edge_color, float edge_size)
{
    get_parameter(1)->set_uniform(0, &edge_color, sizeof(float4), 0);
    get_parameter(1)->set_uniform(0, &edge_size, sizeof(float), 16);
}

mmd_edge_material::mmd_edge_material(render_device* device, material_layout* layout)
    : material(device, layout)
{
}

void mmd_edge_material::set_edge(const float4& edge_color, float edge_size)
{
    get_parameter(0)->set_uniform(0, &edge_color, sizeof(float4), 0);
    get_parameter(0)->set_uniform(0, &edge_size, sizeof(float), 16);
}

class color_pass : public rdg_render_pass
{
public:
    color_pass()
    {
        m_color = add_color("color", RHI_TEXTURE_LAYOUT_RENDER_TARGET);
        add_depth_stencil("depth", RHI_TEXTURE_LAYOUT_DEPTH_STENCIL);

        set_shader("mmd-viewer/shaders/color.vert.spv", "mmd-viewer/shaders/color.frag.spv");
        set_cull_mode(RHI_CULL_MODE_NONE);

        set_parameter_layout(
            {engine_parameter_layout::mesh,
             mmd_parameter_layout::material,
             engine_parameter_layout::camera,
             engine_parameter_layout::light},
            1);
    }

    virtual void execute(rhi_command* command, rdg_context* context) override
    {
        rhi_texture_extent extent =
            context->get_texture(m_color->resource->get_index())->get_extent();

        rhi_viewport viewport = {};
        viewport.width = extent.width;
        viewport.height = extent.height;
        viewport.min_depth = 0.0f;
        viewport.max_depth = 1.0f;
        command->set_viewport(viewport);

        rhi_scissor_rect scissor = {};
        scissor.max_x = extent.width;
        scissor.max_y = extent.height;
        command->set_scissor(&scissor, 1);

        command->set_render_pipeline(get_pipeline());

        command->set_render_parameter(2, context->get_camera());
        command->set_render_parameter(3, context->get_light());

        for (const rdg_mesh& mesh : context->get_meshes(this))
        {
            command->set_vertex_buffers(mesh.vertex_buffers, get_input_layout().size());
            command->set_index_buffer(mesh.index_buffer);
            command->set_render_parameter(0, mesh.transform);
            command->set_render_parameter(1, mesh.material);
            command->draw_indexed(mesh.index_start, mesh.index_count, mesh.vertex_start);
        }
    }

private:
    rdg_pass_reference* m_color;
};

class edge_pass : public rdg_render_pass
{
public:
    edge_pass()
    {
        m_color = add_color("color", RHI_TEXTURE_LAYOUT_RENDER_TARGET);
        add_depth_stencil("depth", RHI_TEXTURE_LAYOUT_DEPTH_STENCIL);

        set_shader("mmd-viewer/shaders/edge.vert.spv", "mmd-viewer/shaders/edge.frag.spv");
        set_cull_mode(RHI_CULL_MODE_BACK);

        set_parameter_layout(
            {engine_parameter_layout::mesh,
             mmd_parameter_layout::edge_material,
             engine_parameter_layout::camera},
            1);
    }

    virtual void execute(rhi_command* command, rdg_context* context) override
    {
        rhi_texture_extent extent =
            context->get_texture(m_color->resource->get_index())->get_extent();

        rhi_viewport viewport = {};
        viewport.width = extent.width;
        viewport.height = extent.height;
        viewport.min_depth = 0.0f;
        viewport.max_depth = 1.0f;
        command->set_viewport(viewport);

        rhi_scissor_rect scissor = {};
        scissor.max_x = extent.width;
        scissor.max_y = extent.height;
        command->set_scissor(&scissor, 1);

        command->set_render_pipeline(get_pipeline());

        command->set_render_parameter(2, context->get_camera());
        for (const rdg_mesh& mesh : context->get_meshes(this))
        {
            command->set_vertex_buffers(mesh.vertex_buffers, get_input_layout().size());
            command->set_index_buffer(mesh.index_buffer);
            command->set_render_parameter(0, mesh.transform);
            command->set_render_parameter(1, mesh.material);
            command->draw_indexed(mesh.index_start, mesh.index_count, mesh.vertex_start);
        }
    }

private:
    rdg_pass_reference* m_color;
};

mmd_render_graph::mmd_render_graph(rhi_format render_target_format)
{
    rdg_texture* render_target = add_resource<rdg_texture>("render target", true);
    render_target->set_format(render_target_format);

    rdg_texture* depth_buffer = add_resource<rdg_texture>("depth buffer", true);
    depth_buffer->set_format(RHI_FORMAT_D24_UNORM_S8_UINT);

    color_pass* color = add_pass<color_pass>("color pass");
    edge_pass* edge = add_pass<edge_pass>("edge pass");
    present_pass* present = add_pass<present_pass>("present pass");

    add_edge(render_target, color, "color", RDG_EDGE_OPERATE_CLEAR);
    add_edge(depth_buffer, color, "depth", RDG_EDGE_OPERATE_CLEAR);
    add_edge(color, "color", edge, "color", RDG_EDGE_OPERATE_STORE);
    add_edge(color, "depth", edge, "depth", RDG_EDGE_OPERATE_STORE);
    add_edge(edge, "color", present, "target", RDG_EDGE_OPERATE_STORE);

    m_material_layout =
        std::make_unique<material_layout>(this, std::vector<rdg_pass*>{color, edge});
}
} // namespace violet::sample