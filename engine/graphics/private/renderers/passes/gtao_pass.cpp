#include "graphics/renderers/passes/gtao_pass.hpp"
#include "graphics/resources/hilbert_lut.hpp"

namespace violet
{
struct gtao_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/gtao.hlsl";

    struct constant_data
    {
        std::uint32_t hzb;
        std::uint32_t depth_buffer;
        std::uint32_t normal_buffer;
        std::uint32_t ao_buffer;
        std::uint32_t hilbert_lut;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t slice_count;
        std::uint32_t step_count;
        float radius;
        float falloff;
        std::uint32_t frame;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

void gtao_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv hzb;
        rdg_texture_srv depth_buffer;
        rdg_texture_srv normal_buffer;
        rdg_texture_uav ao_buffer;
        std::uint32_t slice_count;
        std::uint32_t step_count;
        float radius;
        float falloff;
    };

    graph.add_pass<pass_data>(
        "GTAO Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.hzb = pass.add_texture_srv(parameter.hzb, RHI_PIPELINE_STAGE_COMPUTE);
            data.depth_buffer =
                pass.add_texture_srv(parameter.depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.normal_buffer =
                pass.add_texture_srv(parameter.normal_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.ao_buffer = pass.add_texture_uav(parameter.ao_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.slice_count = parameter.slice_count;
            data.step_count = parameter.step_count;
            data.radius = parameter.radius;
            data.falloff = parameter.falloff;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            rhi_texture_extent extent = data.ao_buffer.get_extent();

            command.set_pipeline({
                .compute_shader = device.get_shader<gtao_cs>(),
            });
            command.set_constant(
                gtao_cs::constant_data{
                    .hzb = data.hzb.get_bindless(),
                    .depth_buffer = data.depth_buffer.get_bindless(),
                    .normal_buffer = data.normal_buffer.get_bindless(),
                    .ao_buffer = data.ao_buffer.get_bindless(),
                    .hilbert_lut =
                        device.get_buildin_texture<hilbert_lut>()->get_srv()->get_bindless(),
                    .width = extent.width,
                    .height = extent.height,
                    .slice_count = data.slice_count,
                    .step_count = data.step_count,
                    .radius = data.radius,
                    .falloff = data.falloff,
                    .frame = device.get_frame_count(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_CAMERA);

            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet