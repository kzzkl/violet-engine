#include "graphics/renderers/passes/ssgi_pass.hpp"
#include "graphics/renderers/passes/blit_pass.hpp"
#include "graphics/resources/spatiotemporal_blue_noise.hpp"

namespace violet
{
struct ssgi_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ssgi/ssgi.hlsl";

    struct constant_data
    {
        std::uint32_t scene_color;
        std::uint32_t normal_buffer;
        vec2f hzb_texel_size;
        std::uint32_t hzb;
        std::uint32_t hzb_start_level;
        std::uint32_t hzb_end_level;
        std::uint32_t motion_vector;
        vec2f stbn_cosine_texel_size;
        std::uint32_t stbn_cosine;
        std::uint32_t stbn_cosine_slice;
        std::uint32_t sample_count;
        std::uint32_t irradiance_sh;
        std::uint32_t indirect_diffuse;
        float thickness;
        std::uint32_t iteration_count;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

struct ssgi_temporal_denoise_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ssgi/ssgi_temporal_denoise.hlsl";

    struct constant_data
    {
        std::uint32_t current_buffer;
        std::uint32_t history_buffer;
        std::uint32_t indirect_diffuse;
        std::uint32_t motion_vector;
        vec2f texel_size;
        std::uint32_t history_valid;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct ssgi_bilatral_denoise_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ssgi/ssgi_bilatral_denoise.hlsl";

    struct constant_data
    {
        std::uint32_t src;
        std::uint32_t dst;
        vec2f texel_size;
        float blur_factor;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

void ssgi_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "SSGI");

    m_ssgi_buffer = graph.add_texture(
        "SSGI Buffer",
        parameter.indirect_diffuse->get_extent(),
        RHI_FORMAT_R16G16B16A16_FLOAT,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE);

    add_ssgi_pass(graph, parameter);
    add_temporal_denoise_pass(graph, parameter);

    if (parameter.bilateral_denoise)
    {
        add_bilatral_denoise_pass(graph, parameter);
    }

    add_debug_pass(graph, parameter);
}

void ssgi_pass::add_ssgi_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv scene_color;
        rdg_texture_srv normal_buffer;
        rdg_texture_srv hzb;
        rdg_buffer_srv irradiance_sh;
        rdg_texture_uav indirect_diffuse;
        float thickness;
        std::uint32_t iteration_count;
        std::uint32_t frame;
    };

    graph.add_pass<pass_data>(
        "SSGI Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.scene_color =
                pass.add_texture_srv(parameter.scene_color, RHI_PIPELINE_STAGE_COMPUTE);
            data.normal_buffer =
                pass.add_texture_srv(parameter.normal_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.hzb = pass.add_texture_srv(parameter.hzb, RHI_PIPELINE_STAGE_COMPUTE);
            data.irradiance_sh =
                pass.add_buffer_srv(parameter.irradiance_sh, RHI_PIPELINE_STAGE_COMPUTE);
            data.indirect_diffuse = pass.add_texture_uav(m_ssgi_buffer, RHI_PIPELINE_STAGE_COMPUTE);

            data.thickness = parameter.thickness;
            data.iteration_count = parameter.iteration_count;
            data.frame = parameter.frame;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            auto* stbn_cosine = device.get_texture<spatiotemporal_blue_noise_cosine>();
            std::uint32_t stbn_cosine_slice =
                data.frame % stbn_cosine->get_rhi()->get_layer_count();
            rhi_extent stbn_cosine_extent = stbn_cosine->get_rhi()->get_extent();
            vec2f stbn_cosine_texel_size = {
                1.0f / static_cast<float>(stbn_cosine_extent.width),
                1.0f / static_cast<float>(stbn_cosine_extent.height),
            };

            command.set_pipeline({
                .compute_shader = device.get_shader<ssgi_cs>(),
            });

            rhi_extent hzb_extent = data.hzb.get_extent();
            vec2f hzb_texel_size = {
                1.0f / static_cast<float>(hzb_extent.width),
                1.0f / static_cast<float>(hzb_extent.height),
            };

            command.set_constant(
                ssgi_cs::constant_data{
                    .scene_color = data.scene_color.get_bindless(),
                    .normal_buffer = data.normal_buffer.get_bindless(),
                    .hzb_texel_size = hzb_texel_size,
                    .hzb = data.hzb.get_bindless(),
                    .hzb_start_level = 1,
                    .hzb_end_level = static_cast<std::uint32_t>(data.hzb.get_level_count()) - 1,
                    .stbn_cosine_texel_size = stbn_cosine_texel_size,
                    .stbn_cosine =
                        stbn_cosine->get_srv(RHI_TEXTURE_DIMENSION_2D_ARRAY)->get_bindless(),
                    .stbn_cosine_slice = stbn_cosine_slice,
                    .sample_count = 2,
                    .irradiance_sh = data.irradiance_sh.get_bindless(),
                    .indirect_diffuse = data.indirect_diffuse.get_bindless(),
                    .thickness = data.thickness,
                    .iteration_count = data.iteration_count,
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_CAMERA);

            rhi_extent extent = data.indirect_diffuse.get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });
}

void ssgi_pass::add_temporal_denoise_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv current_buffer;
        rdg_texture_srv history_buffer;
        rdg_texture_uav indirect_diffuse;

        rdg_texture_srv motion_vector;

        bool history_valid;
    };

    graph.add_pass<pass_data>(
        "SSGI Temporal Denoise Pass",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.current_buffer = pass.add_texture_srv(m_ssgi_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.history_buffer =
                pass.add_texture_srv(parameter.history, RHI_PIPELINE_STAGE_COMPUTE);
            data.indirect_diffuse =
                pass.add_texture_uav(parameter.indirect_diffuse, RHI_PIPELINE_STAGE_COMPUTE);
            data.motion_vector =
                pass.add_texture_srv(parameter.motion_vector, RHI_PIPELINE_STAGE_COMPUTE);

            data.history_valid = parameter.history_valid;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<ssgi_temporal_denoise_cs>(),
            });

            rhi_extent extent = data.indirect_diffuse.get_extent();

            command.set_constant(
                ssgi_temporal_denoise_cs::constant_data{
                    .current_buffer = data.current_buffer.get_bindless(),
                    .history_buffer = data.history_buffer.get_bindless(),
                    .indirect_diffuse = data.indirect_diffuse.get_bindless(),
                    .motion_vector = data.motion_vector.get_bindless(),
                    .texel_size =
                        {
                            1.0f / static_cast<float>(extent.width),
                            1.0f / static_cast<float>(extent.height),
                        },
                    .history_valid = data.history_valid ? 1u : 0u,
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_2d(extent.width, extent.height);
        });

    rhi_texture_region region = {
        .extent = parameter.indirect_diffuse->get_extent(),
        .layer_count = 1,
    };

    graph.add_pass<blit_pass>({
        .src = parameter.indirect_diffuse,
        .src_region = region,
        .dst = parameter.history,
        .dst_region = region,
        .filter = RHI_FILTER_POINT,
    });
}

void ssgi_pass::add_bilatral_denoise_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv src;
        rdg_texture_uav dst;
        float blur_factor;
    };

    graph.add_pass<pass_data>(
        "SSGI Bilatral Denoise Pass: Horizontal",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.src = pass.add_texture_srv(parameter.indirect_diffuse, RHI_PIPELINE_STAGE_COMPUTE);
            data.dst = pass.add_texture_uav(m_ssgi_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.blur_factor = parameter.bilateral_blur_factor;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            std::vector<std::wstring> defines = {L"-DBLUR_HORIZONTAL"};

            command.set_pipeline({
                .compute_shader = device.get_shader<ssgi_bilatral_denoise_cs>(defines),
            });

            rhi_extent extent = data.dst.get_extent();

            command.set_constant(
                ssgi_bilatral_denoise_cs::constant_data{
                    .src = data.src.get_bindless(),
                    .dst = data.dst.get_bindless(),
                    .texel_size =
                        {
                            1.0f / static_cast<float>(extent.width),
                            1.0f / static_cast<float>(extent.height),
                        },
                    .blur_factor = data.blur_factor,
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_2d(extent.width, extent.height);
        });

    graph.add_pass<pass_data>(
        "SSGI Bilatral Denoise Pass: Vertical",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.src = pass.add_texture_srv(m_ssgi_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.dst = pass.add_texture_uav(parameter.indirect_diffuse, RHI_PIPELINE_STAGE_COMPUTE);
            data.blur_factor = parameter.bilateral_blur_factor;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<ssgi_bilatral_denoise_cs>(),
            });

            rhi_extent extent = data.dst.get_extent();

            command.set_constant(
                ssgi_bilatral_denoise_cs::constant_data{
                    .src = data.src.get_bindless(),
                    .dst = data.dst.get_bindless(),
                    .texel_size =
                        {
                            1.0f / static_cast<float>(extent.width),
                            1.0f / static_cast<float>(extent.height),
                        },
                    .blur_factor = data.blur_factor,
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_2d(extent.width, extent.height);
        });
}

void ssgi_pass::add_debug_pass(render_graph& graph, const parameter& parameter)
{
    if (parameter.debug_mode != DEBUG_MODE_SSGI || parameter.debug_output == nullptr)
    {
        return;
    }

    rhi_texture_region src_region = {
        .extent = parameter.indirect_diffuse->get_extent(),
        .layer_count = 1,
    };

    rhi_texture_region dst_region = {
        .extent = parameter.debug_output->get_extent(),
        .layer_count = 1,
    };

    graph.add_pass<blit_pass>({
        .src = parameter.indirect_diffuse,
        .src_region = src_region,
        .dst = parameter.debug_output,
        .dst_region = dst_region,
        .filter = RHI_FILTER_POINT,
    });
}
} // namespace violet