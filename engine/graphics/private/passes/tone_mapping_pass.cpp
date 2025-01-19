#include "graphics/passes/tone_mapping_pass.hpp"

namespace violet
{
struct tone_mapping_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/tone_mapping.hlsl";

    struct tone_mapping_data
    {
        std::uint32_t hdr;
        std::uint32_t ldr;
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(tone_mapping_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, parameter},
    };
};

void tone_mapping_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv hdr_texture;
        rdg_texture_uav ldr_texture;
        rhi_parameter* tone_mapping_parameter;
    };

    graph.add_pass<pass_data>(
        "Tone Mapping Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.hdr_texture =
                pass.add_texture_srv(parameter.hdr_texture, RHI_PIPELINE_STAGE_COMPUTE);
            data.ldr_texture =
                pass.add_texture_uav(parameter.ldr_texture, RHI_PIPELINE_STAGE_COMPUTE);
            data.tone_mapping_parameter = pass.add_parameter(tone_mapping_cs::parameter);
        },
        [](const pass_data& data, rdg_command& command)
        {
            tone_mapping_cs::tone_mapping_data tone_mapping_data = {
                .hdr = data.hdr_texture.get_bindless(),
                .ldr = data.ldr_texture.get_bindless(),
            };

            data.tone_mapping_parameter->set_constant(
                0,
                &tone_mapping_data,
                sizeof(tone_mapping_cs::tone_mapping_data));

            rdg_compute_pipeline pipeline = {
                .compute_shader = render_device::instance().get_shader<tone_mapping_cs>(),
            };
            command.set_pipeline(pipeline);
            command.set_parameter(0, render_device::instance().get_bindless_parameter());
            command.set_parameter(1, data.tone_mapping_parameter);

            rhi_texture_extent extent = data.hdr_texture.get_texture()->get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet