#include "pbr_render.hpp"
#include "graphics/passes/skybox_pass.hpp"

namespace violet::sample
{
namespace
{
static const std::string sides[] = {"right", "left", "top", "bottom", "front", "back"};
}

class irradiance_pass : public render_pass
{
public:
    irradiance_pass(renderer* renderer, setup_context& context) : render_pass(renderer, context)
    {
        m_environment_map = add_slot("environment map");
        m_irradiance_map = add_slot("irradiance map");

        rhi_parameter_layout* layout = renderer->add_parameter_layout(
            "irradiance pass",
            {
                {RHI_PARAMETER_TYPE_UNIFORM_BUFFER, 8, RHI_PARAMETER_STAGE_FLAG_FRAGMENT},
                {RHI_PARAMETER_TYPE_TEXTURE,        1, RHI_PARAMETER_STAGE_FLAG_FRAGMENT}
        });
        m_parameter = renderer->create_parameter(layout);

        rhi_sampler_desc sampler_desc = {};
        sampler_desc.min_filter = RHI_FILTER_LINEAR;
        sampler_desc.mag_filter = RHI_FILTER_LINEAR;
        sampler_desc.address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_desc.address_mode_v = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_desc.address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        m_sampler = renderer->create_sampler(sampler_desc);

        std::vector<render_subpass_reference> references;
        for (const std::string& side : sides)
        {
            pass_slot* slot = add_slot(side, true);
            slot->set_format(RHI_RESOURCE_FORMAT_R8G8B8A8_UNORM);
            slot->set_input_layout(RHI_IMAGE_LAYOUT_RENDER_TARGET);
            references.push_back(
                {slot, RHI_ATTACHMENT_REFERENCE_TYPE_COLOR, RHI_IMAGE_LAYOUT_RENDER_TARGET});
        }
        std::size_t subpass = add_subpass(references);

        m_pipeline = add_pipeline(subpass);
        m_pipeline->set_shader(
            "pbr/shaders/irradiance_map.vert.spv",
            "pbr/shaders/irradiance_map.frag.spv");
        m_pipeline->set_parameter_layouts({layout});
        m_pipeline->set_cull_mode(RHI_CULL_MODE_FRONT);
    }

    virtual void execute(execute_context& context) override
    {
        rhi_resource_extent extent = get_extent();

        struct irradiance_data
        {
            std::uint32_t width;
            std::uint32_t height;
        };
        irradiance_data data = {extent.width, extent.height};
        m_parameter->set_uniform(0, &data, sizeof(irradiance_data), 0);
        m_parameter->set_texture(1, m_environment_map->get_image(), m_sampler.get());

        rhi_render_command* command = context.get_command();

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

        command->begin(get_interface(), get_framebuffer());
        command->set_render_pipeline(m_pipeline->get_interface());
        command->set_render_parameter(0, m_parameter.get());
        command->draw(0, 6);
        command->end();
    }

private:
    pass_slot* m_environment_map;
    pass_slot* m_irradiance_map;

    rhi_ptr<rhi_parameter> m_parameter;
    rhi_ptr<rhi_sampler> m_sampler;

    render_pipeline* m_pipeline;
};

preprocess_graph::preprocess_graph(renderer* renderer) : render_graph(renderer)
{
    pass* irradiance = add_pass<irradiance_pass>();

    for (const std::string& side : sides)
    {
        pass_slot* slot = add_slot(side);
        irradiance->get_slot(side)->connect(slot);
    }

    pass_slot* environment_map = add_slot("environment map");
    pass_slot* irradiance_map = add_slot("irradiance map");
    irradiance->get_slot("environment map")->connect(environment_map);
    irradiance->get_slot("irradiance map")->connect(irradiance_map);

    compile();
}

void preprocess_graph::set_target(rhi_image* environment_map, rhi_image* irradiance_map)
{
    get_slot("environment map")->set_image(environment_map, false);
    get_slot("irradiance map")->set_image(irradiance_map, false);

    rhi_resource_extent extent = irradiance_map->get_extent();
    rhi_image_desc irradiance_target_desc = {
        extent.width,
        extent.height,
        RHI_RESOURCE_FORMAT_R8G8B8A8_UNORM,
        RHI_SAMPLE_COUNT_1,
        RHI_IMAGE_FLAG_RENDER_TARGET | RHI_IMAGE_FLAG_TRANSFER_SRC};
    for (std::size_t i = 0; i < 6; ++i)
    {
        m_irradiance_targets[i] = get_renderer()->create_image(irradiance_target_desc);
        get_slot(sides[i])->set_image(m_irradiance_targets[i].get());
    }
}

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

        pass_slot* render_target = add_slot("render target", true);
        pass_slot* depth_buffer = add_slot("depth buffer", true);
        depth_buffer->set_format(RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT);

        std::size_t subpass = add_subpass({
            {render_target, RHI_ATTACHMENT_REFERENCE_TYPE_COLOR, RHI_IMAGE_LAYOUT_RENDER_TARGET},
            {depth_buffer,
             RHI_ATTACHMENT_REFERENCE_TYPE_DEPTH_STENCIL,        RHI_IMAGE_LAYOUT_DEPTH_STENCIL}
        });

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

        rhi_resource_extent extent = get_extent();

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

        command->begin(get_interface(), get_framebuffer());

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

    compile();
}
} // namespace violet::sample