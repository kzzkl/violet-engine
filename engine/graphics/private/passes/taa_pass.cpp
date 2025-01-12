#include "graphics/passes/taa_pass.hpp"

namespace violet
{
struct taa_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/taa.hlsl";

    struct taa_data
    {
        std::uint32_t render_target;
        std::uint32_t history_render_target;
        std::uint32_t depth_buffer;
        std::uint32_t motion_vector;
        std::uint32_t width;
        std::uint32_t height;
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(taa_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, camera},
        {2, parameter},
    };
};

void taa_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture* render_target;
        rdg_texture* history_render_target;
        rdg_texture* depth_buffer;
        rdg_texture* motion_vector;

        rhi_parameter* taa_parameter;
        rhi_parameter* camera_parameter;
    };

    pass_data data = {
        .render_target = parameter.render_target,
        .history_render_target = parameter.history_render_target,
        .depth_buffer = parameter.depth_buffer,
        .motion_vector = parameter.motion_vector,
        .taa_parameter = graph.allocate_parameter(taa_cs::parameter),
        .camera_parameter = parameter.camera.camera_parameter,
    };

    auto& pass = graph.add_pass<rdg_compute_pass>("TAA Pass");
    pass.add_texture(
        data.render_target,
        RHI_PIPELINE_STAGE_COMPUTE,
        RHI_ACCESS_SHADER_WRITE,
        RHI_TEXTURE_LAYOUT_GENERAL);
    pass.add_texture(
        data.history_render_target,
        RHI_PIPELINE_STAGE_COMPUTE,
        RHI_ACCESS_SHADER_READ,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    pass.add_texture(
        data.depth_buffer,
        RHI_PIPELINE_STAGE_COMPUTE,
        RHI_ACCESS_SHADER_READ,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    if (data.motion_vector != nullptr)
    {
        pass.add_texture(
            data.motion_vector,
            RHI_PIPELINE_STAGE_COMPUTE,
            RHI_ACCESS_SHADER_READ,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    }
    pass.set_execute(
        [data](rdg_command& command)
        {
            auto& device = render_device::instance();

            rhi_texture_extent extent = data.render_target->get_extent();

            taa_cs::taa_data taa_data = {
                .render_target = data.render_target->get_handle(),
                .history_render_target = data.history_render_target->get_handle(),
                .depth_buffer = data.depth_buffer->get_handle(),
                .motion_vector = data.motion_vector->get_handle(),
                .width = extent.width,
                .height = extent.height,
            };
            data.taa_parameter->set_constant(0, &taa_data, sizeof(taa_cs::taa_data));

            std::vector<std::wstring> defines;

            if (data.motion_vector != nullptr)
            {
                defines.emplace_back(L"-DUSE_MOTION_VECTOR");
                taa_data.motion_vector = data.motion_vector->get_handle();
            }

            command.set_pipeline({
                .compute_shader = device.get_shader<taa_cs>(defines),
            });

            command.set_parameter(0, device.get_bindless_parameter());
            command.set_parameter(1, data.camera_parameter);
            command.set_parameter(2, data.taa_parameter);

            command.dispatch_2d(extent.width, extent.height);
        });

    auto& copy_pass = graph.add_pass<rdg_pass>("TAA Copy Render Target");
    copy_pass.add_texture(
        data.render_target,
        RHI_PIPELINE_STAGE_TRANSFER,
        RHI_ACCESS_TRANSFER_READ,
        RHI_TEXTURE_LAYOUT_TRANSFER_SRC);
    copy_pass.add_texture(
        data.history_render_target,
        RHI_PIPELINE_STAGE_TRANSFER,
        RHI_ACCESS_TRANSFER_WRITE,
        RHI_TEXTURE_LAYOUT_TRANSFER_DST);
    copy_pass.set_execute(
        [data](rdg_command& command)
        {
            rhi_texture_region region = {
                .extent = data.render_target->get_extent(),
                .layer_count = 1,
            };

            command.copy_texture(
                data.render_target->get_rhi(),
                region,
                data.history_render_target->get_rhi(),
                region);
        });
}
} // namespace violet