#include "graphics/renderers/passes/ssgi_pass.hpp"
#include "graphics/renderers/passes/blit_pass.hpp"
#include "graphics/renderers/passes/hzb_pass.hpp"

namespace violet
{
struct ssgi_radiance_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ssgi.hlsl";

    struct constant_data
    {
        std::uint32_t gbuffers[8];
        std::uint32_t depth_buffer;
        std::uint32_t hzb;
        vec2f hzb_texel_size;
        std::uint32_t hzb_level_count;
        std::uint32_t radiance_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

void ssgi_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "SSGI");

    add_hzb_pass(graph, parameter);
    add_ssgi_pass(graph, parameter);
    add_debug_pass(graph, parameter);
}

void ssgi_pass::add_hzb_pass(render_graph& graph, const parameter& parameter)
{
    rhi_extent extent = parameter.depth_buffer->get_extent();
    extent.width = std::max(1u, extent.width / 2);
    extent.height = std::max(1u, extent.height / 2);

    std::uint32_t hzb_level_count =
        static_cast<std::uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) +
        1;

    m_hzb = graph.add_texture(
        "HZB for SSGI",
        extent,
        RHI_FORMAT_R32_FLOAT,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE,
        hzb_level_count);

    graph.add_pass<hzb_pass>({
        .depth_buffer = parameter.depth_buffer,
        .hzb = m_hzb,
        .mode = hzb_pass::REDUCTION_MODE_MAX,
    });
}

void ssgi_pass::add_ssgi_pass(render_graph& graph, const parameter& parameter)
{
    m_ssgi_radiance = graph.add_texture(
        "SSGI Radiance",
        parameter.render_target->get_extent(),
        RHI_FORMAT_R16G16B16A16_FLOAT,
        RHI_TEXTURE_TRANSFER_SRC | RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE);

    struct pass_data
    {
        rdg_texture_srv depth_buffer;
        rdg_texture_srv hzb;
        rdg_texture_uav radiance_buffer;
    };

    graph.add_pass<pass_data>(
        "SSGI Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.depth_buffer =
                pass.add_texture_srv(parameter.depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.hzb = pass.add_texture_srv(m_hzb, RHI_PIPELINE_STAGE_COMPUTE);
            data.radiance_buffer =
                pass.add_texture_uav(m_ssgi_radiance, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<ssgi_radiance_cs>(),
            });

            rhi_extent hzb_extent = data.hzb.get_extent();
            vec2f hzb_texel_size = {
                1.0f / static_cast<float>(hzb_extent.width),
                1.0f / static_cast<float>(hzb_extent.height),
            };

            command.set_constant(
                ssgi_radiance_cs::constant_data{
                    .gbuffers = {},
                    .depth_buffer = data.depth_buffer.get_bindless(),
                    .hzb = data.hzb.get_bindless(),
                    .hzb_texel_size = hzb_texel_size,
                    .hzb_level_count = static_cast<std::uint32_t>(data.hzb.get_level_count()),
                    .radiance_buffer = data.radiance_buffer.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_CAMERA);

            rhi_extent extent = data.radiance_buffer.get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });
}

void ssgi_pass::add_debug_pass(render_graph& graph, const parameter& parameter)
{
    if (parameter.debug_mode != DEBUG_MODE_SSGI || parameter.debug_output == nullptr)
    {
        return;
    }

    rhi_texture_region region = {
        .extent = parameter.render_target->get_extent(),
        .layer_count = 1,
    };

    graph.add_pass<blit_pass>({
        .src = m_ssgi_radiance,
        .src_region = region,
        .dst = parameter.debug_output,
        .dst_region = region,
        .filter = RHI_FILTER_POINT,
    });
}
} // namespace violet