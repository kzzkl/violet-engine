#include "graphics/renderers/passes/tone_mapping_pass.hpp"

namespace violet
{
struct tone_mapping_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/tone_mapping.hlsl";

    struct constant_data
    {
        std::uint32_t hdr;
        std::uint32_t ldr;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

void tone_mapping_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv hdr_texture;
        rdg_texture_uav ldr_texture;
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
        },
        [](const pass_data& data, rdg_command& command)
        {
            rdg_compute_pipeline pipeline = {
                .compute_shader = render_device::instance().get_shader<tone_mapping_cs>(),
            };
            command.set_pipeline(pipeline);
            command.set_constant(
                tone_mapping_cs::constant_data{
                    .hdr = data.hdr_texture.get_bindless(),
                    .ldr = data.ldr_texture.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            rhi_texture_extent extent = data.hdr_texture.get_texture()->get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet