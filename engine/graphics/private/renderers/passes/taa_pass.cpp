#include "graphics/renderers/passes/taa_pass.hpp"
#include "graphics/renderers/passes/blit_pass.hpp"

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
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

void taa_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "TAA");

    if (parameter.history_valid)
    {
        resolve(graph, parameter);
    }

    rhi_texture_region region = {
        .extent = parameter.current_render_target->get_extent(),
        .layer_count = 1,
    };

    graph.add_pass<blit_pass>({
        .src = parameter.current_render_target,
        .src_region = region,
        .dst = parameter.history_render_target,
        .dst_region = region,
    });
}

void taa_pass::resolve(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_uav current_render_target;
        rdg_texture_srv history_render_target;
        rdg_texture_srv depth_buffer;
        rdg_texture_srv motion_vector;
    };

    graph.add_pass<pass_data>(
        "TAA Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.current_render_target =
                pass.add_texture_uav(parameter.current_render_target, RHI_PIPELINE_STAGE_COMPUTE);
            data.history_render_target =
                pass.add_texture_srv(parameter.history_render_target, RHI_PIPELINE_STAGE_COMPUTE);
            data.depth_buffer =
                pass.add_texture_srv(parameter.depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);

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

            taa_cs::constant_data constant = {
                .current_render_target = data.current_render_target.get_bindless(),
                .history_render_target =
                    data.history_render_target ? data.history_render_target.get_bindless() : 0,
                .depth_buffer = data.depth_buffer.get_bindless(),
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

            rhi_texture_extent extent = data.current_render_target.get_texture()->get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet