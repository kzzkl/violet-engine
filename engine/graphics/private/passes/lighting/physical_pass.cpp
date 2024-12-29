#include "graphics/passes/lighting/physical_pass.hpp"

namespace violet
{
struct physical_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/lighting/physical.hlsl";

    struct gbuffer_data
    {
        std::uint32_t albedo;
        std::uint32_t material;
        std::uint32_t normal;
        std::uint32_t depth;
        std::uint32_t emissive;
    };

    static constexpr parameter gbuffer = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_FRAGMENT,
            .size = sizeof(gbuffer_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, scene},
        {2, camera},
        {3, gbuffer},
    };
};

void physical_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rhi_parameter* scene_parameter;
        rhi_parameter* camera_parameter;
        rhi_parameter* gbuffer_parameter;

        rdg_texture* gbuffer_albedo;
        rdg_texture* gbuffer_material;
        rdg_texture* gbuffer_normal;
        rdg_texture* gbuffer_depth;
        rdg_texture* gbuffer_emissive;
    };

    pass_data data = {
        .scene_parameter = parameter.scene.get_scene_parameter(),
        .camera_parameter = parameter.camera.camera_parameter,
        .gbuffer_parameter = graph.allocate_parameter(physical_fs::gbuffer),
        .gbuffer_albedo = parameter.gbuffer_albedo,
        .gbuffer_material = parameter.gbuffer_material,
        .gbuffer_normal = parameter.gbuffer_normal,
        .gbuffer_depth = parameter.gbuffer_depth,
        .gbuffer_emissive = parameter.gbuffer_emissive,
    };

    auto& pass = graph.add_pass<rdg_render_pass>("Physical Pass");
    pass.add_texture(
        parameter.gbuffer_albedo,
        RHI_PIPELINE_STAGE_FRAGMENT,
        RHI_ACCESS_SHADER_READ,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    pass.add_texture(
        parameter.gbuffer_material,
        RHI_PIPELINE_STAGE_FRAGMENT,
        RHI_ACCESS_SHADER_READ,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    pass.add_texture(
        parameter.gbuffer_normal,
        RHI_PIPELINE_STAGE_FRAGMENT,
        RHI_ACCESS_SHADER_READ,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    pass.add_texture(
        parameter.gbuffer_depth,
        RHI_PIPELINE_STAGE_FRAGMENT,
        RHI_ACCESS_SHADER_READ,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    pass.add_render_target(
        parameter.render_target,
        parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
    pass.set_depth_stencil(parameter.depth_buffer, RHI_ATTACHMENT_LOAD_OP_LOAD);
    pass.set_execute(
        [data](rdg_command& command)
        {
            physical_fs::gbuffer_data gbuffer_data = {
                .albedo = data.gbuffer_albedo->get_handle(),
                .material = data.gbuffer_material->get_handle(),
                .normal = data.gbuffer_normal->get_handle(),
                .depth = data.gbuffer_depth->get_handle(),
                .emissive = data.gbuffer_emissive->get_handle(),
            };
            data.gbuffer_parameter->set_constant(
                0,
                &gbuffer_data,
                sizeof(physical_fs::gbuffer_data));

            auto& device = render_device::instance();

            rdg_render_pipeline pipeline = {
                .vertex_shader = device.get_shader<fullscreen_vs>(),
                .fragment_shader = device.get_shader<physical_fs>(),
            };
            pipeline.depth_stencil.stencil_enable = true;
            pipeline.depth_stencil.stencil_front = {
                .compare_op = RHI_COMPARE_OP_EQUAL,
                .reference = SHADING_MODEL_PHYSICAL,
            };
            pipeline.depth_stencil.stencil_back = pipeline.depth_stencil.stencil_front;

            command.set_pipeline(pipeline);
            command.set_parameter(0, device.get_bindless_parameter());
            command.set_parameter(1, data.scene_parameter);
            command.set_parameter(2, data.camera_parameter);
            command.set_parameter(3, data.gbuffer_parameter);
            command.draw_fullscreen();
        });
}
} // namespace violet