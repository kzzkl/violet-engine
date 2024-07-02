#include "pbr_render.hpp"
#include "graphics/passes/skybox_pass.hpp"

namespace violet::sample
{
namespace
{
static const std::string sides[] = {"right", "left", "top", "bottom", "front", "back"};
}

class irradiance_pass : public rdg_render_pass
{
public:
    irradiance_pass()
    {
        for (std::size_t i = 0; i < m_irradiance_maps.size(); ++i)
            m_irradiance_maps[i] = add_color(sides[i], RHI_TEXTURE_LAYOUT_RENDER_TARGET);
        m_environment_map = add_texture(
            "environment map",
            RDG_PASS_ACCESS_FLAG_READ,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        set_shader("pbr/shaders/irradiance_map.vert.spv", "pbr/shaders/irradiance_map.frag.spv");
        set_cull_mode(RHI_CULL_MODE_FRONT);

        set_parameter_layout({engine_parameter_layout::camera});
    }

    virtual void compile(render_device* device) override
    {
        rhi_parameter_desc parameter_desc = {};
        parameter_desc.bindings[0] = {
            .type = RHI_PARAMETER_TYPE_UNIFORM_BUFFER,
            .stage = RHI_PARAMETER_STAGE_FLAG_FRAGMENT,
            .size = 8};
        parameter_desc.bindings[1] = {
            .type = RHI_PARAMETER_TYPE_TEXTURE,
            .stage = RHI_PARAMETER_STAGE_FLAG_FRAGMENT,
            .size = 1};
        m_parameter = device->create_parameter(parameter_desc);

        rhi_sampler_desc sampler_desc = {};
        m_sampler = device->create_sampler(sampler_desc);
    }

    virtual void execute(rhi_command* command, rdg_context* context) override
    {
        rhi_texture_extent extent = get_texture(context, m_irradiance_maps[0])->get_extent();

        struct irradiance_data
        {
            std::uint32_t width;
            std::uint32_t height;
        };
        irradiance_data data = {extent.width, extent.height};
        m_parameter->set_uniform(0, &data, sizeof(irradiance_data), 0);
        m_parameter->set_texture(1, get_texture(context, m_environment_map), m_sampler.get());

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

        command->set_render_pipeline(get_pipeline());
        command->set_render_parameter(0, m_parameter.get());
        command->draw(0, 6);
    }

private:
    rdg_pass_reference* m_environment_map;
    std::array<rdg_pass_reference*, 6> m_irradiance_maps;

    rhi_ptr<rhi_parameter> m_parameter;
    rhi_ptr<rhi_sampler> m_sampler;
};

preprocess_graph::preprocess_graph()
{
    rdg_pass* irradiance = add_pass<irradiance_pass>("irradiance pass");

    for (const std::string& side : sides)
    {
        rdg_texture* texture = add_resource<rdg_texture>(side, true);
        add_edge(texture, irradiance, side, RDG_EDGE_ACTION_CLEAR);
    }

    rdg_texture* environment_map = add_resource<rdg_texture>("environment map", true);
    add_edge(environment_map, irradiance, "", RDG_EDGE_ACTION_LOAD);
}

pbr_pass::pbr_pass()
{
    m_render_target = add_color("render target", RHI_TEXTURE_LAYOUT_RENDER_TARGET);
    add_depth_stencil("depth buffer", RHI_TEXTURE_LAYOUT_DEPTH_STENCIL);

    set_shader("pbr/shaders/pbr.vert.spv", "pbr/shaders/pbr.frag.spv");
    set_parameter_layout(
        {engine_parameter_layout::mesh,
         get_material_layout(),
         engine_parameter_layout::camera,
         engine_parameter_layout::light},
        1);
}

void pbr_pass::execute(rhi_command* command, rdg_context* context)
{
    rhi_texture_extent extent = get_texture(context, m_render_target)->get_extent();

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

    command->set_render_pipeline(get_pipeline());
    command->set_render_parameter(2, context->get_camera());
    command->set_render_parameter(3, context->get_light());

    for (const rdg_mesh& mesh : context->get_meshes(this))
    {
        command->set_vertex_buffers(mesh.vertex_buffers, 1);
        command->set_index_buffer(mesh.index_buffer);
        command->set_render_parameter(0, mesh.transform);
        command->set_render_parameter(1, mesh.material);
        command->draw_indexed(mesh.index_start, mesh.index_count, mesh.vertex_start);
    }

    command->end();
}

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

pbr_render_graph::pbr_render_graph()
{
    pass_slot* depth_buffer = add_slot("depth buffer");
    depth_buffer->set_format(RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT);
    depth_buffer->set_samples(RHI_SAMPLE_COUNT_1);

    pass* pbr = add_pass<pbr_pass>();
    pass* skybox = add_pass<skybox_pass>();

    pbr->get_slot("render target")->connect(get_slot("back buffer input"));
    pbr->get_slot("depth buffer")->connect(depth_buffer);

    skybox->get_slot("render target")->connect(pbr->get_slot("render target"));
    skybox->get_slot("depth buffer")->connect(pbr->get_slot("depth buffer"));

    get_slot("back buffer output")->connect(skybox->get_slot("render target"));
}
} // namespace violet::sample