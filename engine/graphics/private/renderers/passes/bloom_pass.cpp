#include "graphics/renderers/passes/bloom_pass.hpp"
#include <format>

namespace violet
{
struct bloom_prefilter_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/bloom/prefilter.hlsl";

    struct constant_data
    {
        vec3f curve; // (threshold - knee, knee * 2, 0.25 / knee)
        float threshold;
        std::uint32_t src;
        std::uint32_t dst;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct bloom_downsample_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/bloom/downsample.hlsl";

    struct constant_data
    {
        vec2f texel_size;
        std::uint32_t src;
        std::uint32_t dst;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct bloom_blur_constant_data
{
    std::uint32_t src;
    std::uint32_t dst;
    float texel_size;
};

struct bloom_blur_horizontal_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/bloom/blur.hlsl";
    static constexpr std::string_view entry_point = "blur_horizontal";

    using constant_data = bloom_blur_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct bloom_blur_vertical_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/bloom/blur.hlsl";
    static constexpr std::string_view entry_point = "blur_vertical";

    using constant_data = bloom_blur_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct bloom_upsample_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/bloom/upsample.hlsl";

    struct constant_data
    {
        vec2f prev_texel_size;
        std::uint32_t prev_src;
        std::uint32_t curr_src;
        std::uint32_t dst;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct bloom_merge_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/bloom/merge.hlsl";

    struct constant_data
    {
        std::uint32_t render_target;
        std::uint32_t bloom;
        float intensity;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct bloom_debug_constant_data
{
    std::uint32_t src;
    std::uint32_t debug_output;
    float intensity;
};

struct bloom_debug_prefilter_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/bloom/bloom_debug.hlsl";
    static constexpr std::string_view entry_point = "debug_prefilter";

    using constant_data = bloom_debug_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct bloom_debug_bloom_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/bloom/bloom_debug.hlsl";
    static constexpr std::string_view entry_point = "debug_bloom";

    using constant_data = bloom_debug_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

void bloom_pass::add(render_graph& graph, const parameter& parameter)
{
    rhi_texture_extent extent = parameter.render_target->get_extent();
    extent.width /= 2;
    extent.height /= 2;

    m_downsample_chain = graph.add_texture(
        "Bloom Downsample Chain",
        extent,
        RHI_FORMAT_R11G11B10_FLOAT,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE,
        bloom_mip_count);

    m_upsample_chain = graph.add_texture(
        "Bloom Upsample Chain",
        extent,
        RHI_FORMAT_R11G11B10_FLOAT,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE,
        bloom_mip_count);

    rdg_scope scope(graph, "Bloom");

    prefilter(graph, parameter);
    downsample(graph);
    blur(graph);
    upsample(graph);
    merge(graph, parameter);
}

void bloom_pass::prefilter(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv src;
        rdg_texture_uav dst;
        float threshold;
        float knee;
    };

    graph.add_pass<pass_data>(
        "Prefilter",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.src = pass.add_texture_srv(parameter.render_target, RHI_PIPELINE_STAGE_COMPUTE);
            data.dst = pass.add_texture_uav(
                m_downsample_chain,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_TEXTURE_DIMENSION_2D,
                0,
                1);

            data.threshold = parameter.threshold;
            data.knee = std::max(parameter.knee, 0.00001f);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<bloom_prefilter_cs>(),
            });

            command.set_constant(
                bloom_prefilter_cs::constant_data{
                    .curve = vec3f(data.threshold - data.knee, data.knee * 2, 0.25f / data.knee),
                    .threshold = data.threshold,
                    .src = data.src.get_bindless(),
                    .dst = data.dst.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            auto extent = data.dst.get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });

    if (parameter.debug_mode == DEBUG_MODE_PREFILTER)
    {
        debug(graph, parameter);
    }
}

void bloom_pass::downsample(render_graph& graph)
{
    rdg_scope scope(graph, "Downsample");

    struct pass_data
    {
        rdg_texture_srv src;
        rdg_texture_uav dst;
    };

    for (std::uint32_t i = 1; i < m_downsample_chain->get_level_count(); ++i)
    {
        graph.add_pass<pass_data>(
            std::format("Level {}", i),
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.src = pass.add_texture_srv(
                    m_downsample_chain,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    i - 1,
                    1);
                data.dst = pass.add_texture_uav(
                    m_downsample_chain,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    i,
                    1);
            },
            [](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<bloom_downsample_cs>(),
                });

                auto extent = data.dst.get_extent();

                command.set_constant(
                    bloom_downsample_cs::constant_data{
                        .texel_size = vec2f(
                            1.0f / static_cast<float>(extent.width),
                            1.0f / static_cast<float>(extent.height)),
                        .src = data.src.get_bindless(),
                        .dst = data.dst.get_bindless(),
                    });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                command.dispatch_2d(extent.width, extent.height);
            });
    }
}

void bloom_pass::blur(render_graph& graph)
{
    rdg_scope scope(graph, "Blur");

    struct pass_data
    {
        rdg_texture_srv src;
        rdg_texture_uav dst;
    };

    for (std::uint32_t level = 0; level < bloom_mip_count; ++level)
    {
        graph.add_pass<pass_data>(
            std::format("Blur Horizontal: Level {}", level),
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.src = pass.add_texture_srv(
                    m_downsample_chain,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    level,
                    1);
                data.dst = pass.add_texture_uav(
                    m_upsample_chain,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    level,
                    1);
            },
            [](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<bloom_blur_horizontal_cs>(),
                });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                auto extent = data.dst.get_extent();

                command.set_constant(
                    bloom_blur_constant_data{
                        .src = data.src.get_bindless(),
                        .dst = data.dst.get_bindless(),
                        .texel_size = 1.0f / static_cast<float>(extent.width),
                    });

                command.dispatch_2d(extent.width, extent.height);
            });

        graph.add_pass<pass_data>(
            std::format("Blur Vertical: Level {}", level),
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.src = pass.add_texture_srv(
                    m_upsample_chain,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    level,
                    1);
                data.dst = pass.add_texture_uav(
                    m_downsample_chain,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    level,
                    1);
            },
            [](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<bloom_blur_vertical_cs>(),
                });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                auto extent = data.dst.get_extent();

                command.set_constant(
                    bloom_blur_constant_data{
                        .src = data.src.get_bindless(),
                        .dst = data.dst.get_bindless(),
                        .texel_size = 1.0f / static_cast<float>(extent.height),
                    });

                command.dispatch_2d(extent.width, extent.height);
            });
    }
}

void bloom_pass::upsample(render_graph& graph)
{
    rdg_scope scope(graph, "Upsample");

    struct pass_data
    {
        rdg_texture_srv prev_src;
        rdg_texture_srv curr_src;
        rdg_texture_uav dst;
    };

    for (std::int32_t level = bloom_mip_count - 2; level >= 0; --level)
    {
        graph.add_pass<pass_data>(
            std::format("Upsample: Level {}", level),
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.prev_src = pass.add_texture_srv(
                    level == bloom_mip_count - 2 ? m_downsample_chain : m_upsample_chain,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    level + 1,
                    1);
                data.curr_src = pass.add_texture_srv(
                    m_downsample_chain,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    level,
                    1);
                data.dst = pass.add_texture_uav(
                    m_upsample_chain,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    level,
                    1);
            },
            [](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<bloom_upsample_cs>(),
                });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                auto prev_extent = data.prev_src.get_extent();
                command.set_constant(
                    bloom_upsample_cs::constant_data{
                        .prev_texel_size = vec2f(
                            1.0f / static_cast<float>(prev_extent.width),
                            1.0f / static_cast<float>(prev_extent.height)),
                        .prev_src = data.prev_src.get_bindless(),
                        .curr_src = data.curr_src.get_bindless(),
                        .dst = data.dst.get_bindless(),
                    });

                auto extent = data.dst.get_extent();
                command.dispatch_2d(extent.width, extent.height);
            });
    }
}

void bloom_pass::merge(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_uav render_target;
        rdg_texture_srv bloom;
        float intensity;
    };

    graph.add_pass<pass_data>(
        "Merge",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.render_target =
                pass.add_texture_uav(parameter.render_target, RHI_PIPELINE_STAGE_COMPUTE);
            data.bloom = pass.add_texture_srv(
                m_upsample_chain,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_TEXTURE_DIMENSION_2D,
                0,
                1);
            data.intensity = parameter.intensity;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<bloom_merge_cs>(),
            });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.set_constant(
                bloom_merge_cs::constant_data{
                    .render_target = data.render_target.get_bindless(),
                    .bloom = data.bloom.get_bindless(),
                    .intensity = data.intensity,
                });

            auto extent = data.render_target.get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });

    if (parameter.debug_mode == DEBUG_MODE_BLOOM)
    {
        debug(graph, parameter);
    }
}

void bloom_pass::debug(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv src;
        rdg_texture_uav debug_output;
        float intensity;
    };

    if (parameter.debug_mode == DEBUG_MODE_BLOOM)
    {
        graph.add_pass<pass_data>(
            "Debug Bloom",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.src = pass.add_texture_srv(
                    m_upsample_chain,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    0,
                    1);
                data.debug_output =
                    pass.add_texture_uav(parameter.debug_output, RHI_PIPELINE_STAGE_COMPUTE);
                data.intensity = parameter.intensity;
            },
            [](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<bloom_debug_bloom_cs>(),
                });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                command.set_constant(
                    bloom_debug_bloom_cs::constant_data{
                        .src = data.src.get_bindless(),
                        .debug_output = data.debug_output.get_bindless(),
                        .intensity = data.intensity,
                    });

                auto extent = data.debug_output.get_extent();
                command.dispatch_2d(extent.width, extent.height);
            });
    }
    else if (parameter.debug_mode == DEBUG_MODE_PREFILTER)
    {
        graph.add_pass<pass_data>(
            "Debug Prefilter",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.src = pass.add_texture_srv(
                    m_downsample_chain,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    0,
                    1);
                data.debug_output =
                    pass.add_texture_uav(parameter.debug_output, RHI_PIPELINE_STAGE_COMPUTE);
            },
            [](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<bloom_debug_prefilter_cs>(),
                });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                command.set_constant(
                    bloom_debug_prefilter_cs::constant_data{
                        .src = data.src.get_bindless(),
                        .debug_output = data.debug_output.get_bindless(),
                    });

                auto extent = data.debug_output.get_extent();
                command.dispatch_2d(extent.width, extent.height);
            });
    }
}
} // namespace violet