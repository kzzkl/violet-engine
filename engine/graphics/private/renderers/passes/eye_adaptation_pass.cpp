#include "graphics/renderers/passes/eye_adaptation_pass.hpp"
#include "graphics/renderers/passes/blit_pass.hpp"

namespace violet
{
struct histogram_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/eye_adaptation/histogram.hlsl";

    struct constant_data
    {
        vec2f scale_offset;
        std::uint32_t render_target;
        std::uint32_t histogram;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct eye_adaptation_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/eye_adaptation/eye_adaptation.hlsl";

    struct constant_data
    {
        vec2f scale_offset;
        std::uint32_t histogram;
        std::uint32_t exposure;
        float low_percent;
        float high_percent;
        float min_brightness;
        float max_brightness;
        float speed_down;
        float speed_up;
        float delta_time;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct apply_exposure_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/eye_adaptation/apply_exposure.hlsl";

    struct constant_data
    {
        std::uint32_t render_target;
        std::uint32_t exposure;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct eye_adaptation_debug_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/eye_adaptation/debug.hlsl";

    struct constant_data
    {
        std::uint32_t render_target;
        std::uint32_t histogram;
        std::uint32_t debug_output;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

void eye_adaptation_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Eye Adaptation");

    m_scale_offset.x = 1.0f / (parameter.max_ev - parameter.min_ev);
    m_scale_offset.y = -parameter.min_ev * m_scale_offset.x;

    prepare(graph, parameter);
    histogram(graph, parameter);
    eye_adaptation(graph, parameter);
    apply_exposure(graph, parameter);

    if (parameter.debug_output != nullptr)
    {
        add_debug_pass(graph, parameter);
    }
}

void eye_adaptation_pass::prepare(render_graph& graph, const parameter& parameter)
{
    rhi_extent extent = parameter.render_target->get_extent();
    extent.width /= 2;
    extent.height /= 2;

    m_render_target = graph.add_texture(
        "Eye Adaptation Input",
        extent,
        RHI_FORMAT_R11G11B10_FLOAT,
        RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE | RHI_TEXTURE_TRANSFER_DST);

    graph.add_pass<blit_pass>({
        .src = parameter.render_target,
        .src_region =
            rhi_texture_region{
                .extent = parameter.render_target->get_extent(),
                .layer_count = 1,
            },
        .dst = m_render_target,
        .dst_region =
            rhi_texture_region{
                .extent = extent,
                .layer_count = 1,
            },
        .filter = RHI_FILTER_LINEAR,
    });

    m_histogram = graph.add_buffer(
        "Eye Adaptation Histogram",
        sizeof(std::uint32_t) * 64,
        RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);

    struct pass_data
    {
        rdg_buffer_ref histogram;
    };

    graph.add_pass<pass_data>(
        "Clear Histogram",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.histogram = pass.add_buffer(
                m_histogram,
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_ACCESS_TRANSFER_WRITE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            rhi_buffer_region region = {
                .size = data.histogram.get_size(),
            };
            command.fill_buffer(data.histogram.get_rhi(), region, 0);
        });
}

void eye_adaptation_pass::histogram(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv render_target;
        rdg_buffer_uav histogram;

        vec2f scale_offset;
    };

    graph.add_pass<pass_data>(
        "Histogram",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.render_target = pass.add_texture_srv(m_render_target, RHI_PIPELINE_STAGE_COMPUTE);
            data.histogram = pass.add_buffer_uav(m_histogram, RHI_PIPELINE_STAGE_COMPUTE);
            data.scale_offset = m_scale_offset;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<histogram_cs>(),
            });

            command.set_constant(
                histogram_cs::constant_data{
                    .scale_offset = data.scale_offset,
                    .render_target = data.render_target.get_bindless(),
                    .histogram = data.histogram.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            rhi_extent extent = data.render_target.get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });
}

void eye_adaptation_pass::eye_adaptation(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_uav exposure;
        rdg_buffer_srv histogram;

        vec2f scale_offset;
        float low_percent;
        float high_percent;
        float min_brightness;
        float max_brightness;
        float speed_down;
        float speed_up;
        float delta_time;
    };

    graph.add_pass<pass_data>(
        "Eye Adaptation",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.exposure = pass.add_texture_uav(parameter.exposure, RHI_PIPELINE_STAGE_COMPUTE);
            data.histogram = pass.add_buffer_srv(m_histogram, RHI_PIPELINE_STAGE_COMPUTE);

            data.scale_offset = m_scale_offset;
            data.low_percent = parameter.low_percent;
            data.high_percent = parameter.high_percent;
            data.min_brightness = parameter.min_brightness;
            data.max_brightness = parameter.max_brightness;
            data.speed_down = parameter.speed_down;
            data.speed_up = parameter.speed_up;
            data.delta_time = parameter.delta_time;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<eye_adaptation_cs>(),
            });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.set_constant(
                eye_adaptation_cs::constant_data{
                    .scale_offset = data.scale_offset,
                    .histogram = data.histogram.get_bindless(),
                    .exposure = data.exposure.get_bindless(),
                    .low_percent = data.low_percent,
                    .high_percent = data.high_percent,
                    .min_brightness = data.min_brightness,
                    .max_brightness = data.max_brightness,
                    .speed_down = data.speed_down,
                    .speed_up = data.speed_up,
                    .delta_time = data.delta_time,
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch(1, 1, 1);
        });
}

void eye_adaptation_pass::apply_exposure(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_uav render_target;
        rdg_texture_srv exposure;
    };

    graph.add_pass<pass_data>(
        "Apply Exposure",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.render_target =
                pass.add_texture_uav(parameter.render_target, RHI_PIPELINE_STAGE_COMPUTE);
            data.exposure = pass.add_texture_srv(parameter.exposure, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<apply_exposure_cs>(),
            });

            command.set_constant(
                apply_exposure_cs::constant_data{
                    .render_target = data.render_target.get_bindless(),
                    .exposure = data.exposure.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            rhi_extent extent = data.render_target.get_extent();
            command.dispatch_2d(extent.width, extent.height, 16, 16);
        });
}

void eye_adaptation_pass::add_debug_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv render_target;
        rdg_buffer_srv histogram;
        rdg_texture_uav debug_output;
    };

    graph.add_pass<pass_data>(
        "Eye Adaptation Debug",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.render_target =
                pass.add_texture_srv(parameter.render_target, RHI_PIPELINE_STAGE_COMPUTE);
            data.histogram = pass.add_buffer_srv(m_histogram, RHI_PIPELINE_STAGE_COMPUTE);
            data.debug_output =
                pass.add_texture_uav(parameter.debug_output, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<eye_adaptation_debug_cs>(),
            });

            command.set_constant(
                eye_adaptation_debug_cs::constant_data{
                    .render_target = data.render_target.get_bindless(),
                    .histogram = data.histogram.get_bindless(),
                    .debug_output = data.debug_output.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            rhi_extent extent = data.debug_output.get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });
}
} // namespace violet