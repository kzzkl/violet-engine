#include "graphics/renderers/passes/taa_pass.hpp"

namespace violet
{
struct taa_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/taa.hlsl";

    struct constant_data
    {
        std::uint32_t current_render_target;
        std::uint32_t history_render_target;
        std::uint32_t depth_buffer;
        std::uint32_t motion_vector;
        std::uint32_t resolved_render_target;
        std::uint32_t width;
        std::uint32_t height;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

void taa_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv current_render_target;
        rdg_texture_srv history_render_target;
        rdg_texture_srv depth_buffer;
        rdg_texture_srv motion_vector;
        rdg_texture_uav resolved_render_target;
    };

    graph.add_pass<pass_data>(
        "TAA Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.current_render_target =
                pass.add_texture_srv(parameter.current_render_target, RHI_PIPELINE_STAGE_COMPUTE);

            if (parameter.history_render_target != nullptr)
            {
                data.history_render_target = pass.add_texture_srv(
                    parameter.history_render_target,
                    RHI_PIPELINE_STAGE_COMPUTE);
            }
            else
            {
                data.history_render_target.reset();
            }

            data.depth_buffer =
                pass.add_texture_srv(parameter.depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.resolved_render_target =
                pass.add_texture_uav(parameter.resolved_render_target, RHI_PIPELINE_STAGE_COMPUTE);

            if (parameter.motion_vector != nullptr)
            {
                data.motion_vector =
                    pass.add_texture_srv(parameter.motion_vector, RHI_PIPELINE_STAGE_COMPUTE);
            }
            else
            {
                data.motion_vector.reset();
            }
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            rhi_texture_extent extent = data.current_render_target.get_texture()->get_extent();

            taa_cs::constant_data constant = {
                .current_render_target = data.current_render_target.get_bindless(),
                .history_render_target =
                    data.history_render_target ? data.history_render_target.get_bindless() : 0,
                .depth_buffer = data.depth_buffer.get_bindless(),
                .resolved_render_target = data.resolved_render_target.get_bindless(),
                .width = extent.width,
                .height = extent.height,
            };

            std::vector<std::wstring> defines;

            if (data.motion_vector)
            {
                defines.emplace_back(L"-DUSE_MOTION_VECTOR");
                constant.motion_vector = data.motion_vector.get_bindless();
            }

            command.set_pipeline({
                .compute_shader = device.get_shader<taa_cs>(defines),
            });
            command.set_constant(constant);
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_CAMERA);

            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet