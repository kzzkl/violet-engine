#include "pbr_render.hpp"
#include "graphics/pipeline/skybox_pipeline.hpp"

namespace violet::sample
{
// Compute diffuse irradiance cubemap
class irradiance_map_pipeline : public render_pipeline
{
public:
    irradiance_map_pipeline() {}

    virtual bool compile(compile_context& context) override
    {
        set_shader("pbr/shaders/irradiance_map.comp.spv", "");
        set_parameter_layouts({
            {context.renderer->get_parameter_layout("pbr pre-process"),
             RENDER_PIPELINE_PARAMETER_TYPE_NORMAL}
        });

        return render_pipeline::compile(context);
    }

    void set_parameter(
        rhi_image* ambient_map,
        rhi_sampler* ambient_sampler,
        rhi_image* irradiance_map)
    {
    }

private:
    virtual void render(std::vector<render_mesh>& meshes, const execute_context& context) override
    {
    }
};

pre_process_graph::pre_process_graph(renderer* renderer) : render_graph(renderer)
{
    rhi_parameter_layout* parameter_layout = renderer->add_parameter_layout(
        "pbr pre-process",
        {
            {RHI_PARAMETER_TYPE_TEXTURE,        1, RHI_PARAMETER_STAGE_FLAG_FRAGMENT},
            {RHI_PARAMETER_TYPE_UNIFORM_BUFFER, 1, RHI_PARAMETER_STAGE_FLAG_VERTEX  }
    });
    m_parameter = renderer->create_parameter(parameter_layout);

    render_pass* pre_process = add_render_pass("pre process");
    render_attachment* output_attachment = pre_process->add_attachment("output");

    render_subpass* irradiance_map_pass = pre_process->add_subpass("irradiance map");
    irradiance_map_pass->add_reference(
        output_attachment,
        RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
        RHI_IMAGE_LAYOUT_RENDER_TARGET);

    irradiance_map_pass->add_pipeline<irradiance_map_pipeline>("irradiance process");

    compile();
}

void pre_process_graph::set_parameter(
    rhi_image* ambient_map,
    rhi_sampler* ambient_sampler,
    rhi_image* irradiance_map)
{
    m_parameter->set_texture(0, ambient_map, ambient_sampler);
    // m_parameter->set_storage(1, irradiance_map);

    compute_pipeline* pipeline = get_compute_pipeline("irradiance process");
    rhi_resource_extent extent = ambient_map->get_extent();
    pipeline->add_dispatch(extent.width / 32, extent.height / 32, 6, {m_parameter.get()});
}

class pbr_pipeline : public render_pipeline
{
public:
    pbr_pipeline() {}

    virtual bool compile(compile_context& context) override
    {
        set_shader("pbr/shaders/pbr.vert.spv", "pbr/shaders/pbr.frag.spv");
        set_vertex_attributes({
            {"position", RHI_RESOURCE_FORMAT_R32G32B32_FLOAT},
            {"normal",   RHI_RESOURCE_FORMAT_R32G32B32_FLOAT}
        });
        set_cull_mode(RHI_CULL_MODE_BACK);

        set_parameter_layouts({
            {context.renderer->get_parameter_layout("violet mesh"),
             RENDER_PIPELINE_PARAMETER_TYPE_MESH    },
            {context.renderer->get_parameter_layout("pbr material"),
             RENDER_PIPELINE_PARAMETER_TYPE_MATERIAL},
            {context.renderer->get_parameter_layout("violet camera"),
             RENDER_PIPELINE_PARAMETER_TYPE_CAMERA  },
            {context.renderer->get_parameter_layout("violet light"),
             RENDER_PIPELINE_PARAMETER_TYPE_NORMAL  }
        });

        return render_pipeline::compile(context);
    }

private:
    virtual void render(std::vector<render_mesh>& meshes, const execute_context& context) override
    {
        context.command->set_render_parameter(2, context.camera);
        context.command->set_render_parameter(3, context.light);

        for (render_mesh& mesh : meshes)
        {
            context.command->set_vertex_buffers(
                mesh.vertex_buffers.data(),
                mesh.vertex_buffers.size());
            context.command->set_index_buffer(mesh.index_buffer);
            context.command->set_render_parameter(0, mesh.transform);
            context.command->set_render_parameter(1, mesh.material);
            context.command->draw_indexed(mesh.index_start, mesh.index_count, mesh.vertex_start);
        }
    }
};

pbr_render_graph::pbr_render_graph(renderer* renderer) : render_graph(renderer)
{
    renderer->add_parameter_layout(
        "pbr material",
        {
            {RHI_PARAMETER_TYPE_UNIFORM_BUFFER, 32, RHI_PARAMETER_STAGE_FLAG_FRAGMENT},
            {RHI_PARAMETER_TYPE_TEXTURE,        1,  RHI_PARAMETER_STAGE_FLAG_FRAGMENT}
    });

    render_pass* main = add_render_pass("main");

    render_attachment* output_attachment = main->add_attachment("output");
    output_attachment->set_format(renderer->get_back_buffer()->get_format());
    output_attachment->set_initial_layout(RHI_IMAGE_LAYOUT_UNDEFINED);
    output_attachment->set_final_layout(RHI_IMAGE_LAYOUT_PRESENT);
    output_attachment->set_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
    output_attachment->set_store_op(RHI_ATTACHMENT_STORE_OP_STORE);

    render_attachment* depth_stencil_attachment = main->add_attachment("depth stencil");
    depth_stencil_attachment->set_format(RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT);
    depth_stencil_attachment->set_initial_layout(RHI_IMAGE_LAYOUT_UNDEFINED);
    depth_stencil_attachment->set_final_layout(RHI_IMAGE_LAYOUT_DEPTH_STENCIL);
    depth_stencil_attachment->set_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
    depth_stencil_attachment->set_store_op(RHI_ATTACHMENT_STORE_OP_DONT_CARE);
    depth_stencil_attachment->set_stencil_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
    depth_stencil_attachment->set_stencil_store_op(RHI_ATTACHMENT_STORE_OP_DONT_CARE);

    render_subpass* color_pass = main->add_subpass("color pass");
    color_pass->add_reference(
        output_attachment,
        RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
        RHI_IMAGE_LAYOUT_RENDER_TARGET);
    color_pass->add_reference(
        depth_stencil_attachment,
        RHI_ATTACHMENT_REFERENCE_TYPE_DEPTH_STENCIL,
        RHI_IMAGE_LAYOUT_DEPTH_STENCIL);

    color_pass->add_pipeline<skybox_pipeline>("skybox");
    color_pass->add_pipeline<pbr_pipeline>("pbr");

    compile();

    auto pbr_material_layout = add_material_layout("pbr");
    pbr_material_layout->add_pipeline(color_pass->get_pipeline("pbr"));
    pbr_material_layout->add_field("albedo", {0, 0, sizeof(float3), 0});
    pbr_material_layout->add_field("metalness", {0, 0, sizeof(float), 12});
    pbr_material_layout->add_field("roughness", {0, 0, sizeof(float), 16});
}
} // namespace violet::sample