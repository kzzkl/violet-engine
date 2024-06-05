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

mmd_color_pass::mmd_color_pass()
{
    add_color(reference_render_target, RHI_TEXTURE_LAYOUT_RENDER_TARGET);
    add_depth_stencil(reference_depth, RHI_TEXTURE_LAYOUT_DEPTH_STENCIL);

    set_shader("mmd-viewer/shaders/color.vert.spv", "mmd-viewer/shaders/color.frag.spv");
    set_cull_mode(RHI_CULL_MODE_NONE);

    set_input_layout({
        {"position", RHI_FORMAT_R32G32B32_FLOAT},
        {"normal",   RHI_FORMAT_R32G32B32_FLOAT},
        {"uv",       RHI_FORMAT_R32G32_FLOAT   }
    });

    set_parameter_layout({
        {engine_parameter_layout::mesh,   RDG_PASS_PARAMETER_FLAG_NONE    },
        {get_material_parameter_layout(), RDG_PASS_PARAMETER_FLAG_MATERIAL},
        {engine_parameter_layout::camera, RDG_PASS_PARAMETER_FLAG_NONE    },
        {engine_parameter_layout::light,  RDG_PASS_PARAMETER_FLAG_NONE    }
    });
}

void mmd_color_pass::execute(rhi_command* command, rdg_context* context)
{
    rhi_texture_extent extent = context->get_texture(this, reference_render_target)->get_extent();

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

mmd_edge_pass::mmd_edge_pass()
{
    add_color(reference_render_target, RHI_TEXTURE_LAYOUT_RENDER_TARGET);
    add_depth_stencil(reference_depth, RHI_TEXTURE_LAYOUT_DEPTH_STENCIL);

    set_shader("mmd-viewer/shaders/edge.vert.spv", "mmd-viewer/shaders/edge.frag.spv");
    set_cull_mode(RHI_CULL_MODE_BACK);

    set_input_layout({
        {"position", RHI_FORMAT_R32G32B32_FLOAT},
        {"normal",   RHI_FORMAT_R32G32B32_FLOAT},
        {"edge",     RHI_FORMAT_R32_FLOAT      }
    });

    set_parameter_layout({
        {engine_parameter_layout::mesh,   RDG_PASS_PARAMETER_FLAG_NONE    },
        {get_material_parameter_layout(), RDG_PASS_PARAMETER_FLAG_MATERIAL},
        {engine_parameter_layout::camera, RDG_PASS_PARAMETER_FLAG_NONE    }
    });
}

void mmd_edge_pass::execute(rhi_command* command, rdg_context* context)
{
    rhi_texture_extent extent = context->get_texture(this, reference_render_target)->get_extent();

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

mmd_render_graph::mmd_render_graph(rhi_format render_target_format)
{
    rdg_texture* render_target = add_resource<rdg_texture>("render target", true);
    render_target->set_format(render_target_format);

    rdg_texture* depth_buffer = add_resource<rdg_texture>("depth buffer", true);
    depth_buffer->set_format(RHI_FORMAT_D24_UNORM_S8_UINT);

    mmd_color_pass* color_pass = add_pass<mmd_color_pass>("color pass");
    mmd_edge_pass* edge_pass = add_pass<mmd_edge_pass>("edge pass");
    present_pass* present = add_pass<present_pass>("present pass");

    add_edge(
        render_target,
        color_pass,
        mmd_color_pass::reference_render_target,
        RDG_EDGE_OPERATE_CLEAR);
    add_edge(depth_buffer, color_pass, mmd_color_pass::reference_depth, RDG_EDGE_OPERATE_CLEAR);
    add_edge(
        color_pass,
        mmd_color_pass::reference_render_target,
        edge_pass,
        mmd_edge_pass::reference_render_target,
        RDG_EDGE_OPERATE_STORE);
    add_edge(
        color_pass,
        mmd_color_pass::reference_depth,
        edge_pass,
        mmd_edge_pass::reference_depth,
        RDG_EDGE_OPERATE_STORE);
    add_edge(
        edge_pass,
        mmd_edge_pass::reference_render_target,
        present,
        present_pass::reference_present_target,
        RDG_EDGE_OPERATE_STORE);

    m_material_layout =
        std::make_unique<material_layout>(this, std::vector<rdg_pass*>{color_pass, edge_pass});
}
} // namespace violet::sample