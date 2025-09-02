#include "graphics/renderers/passes/scan_pass.hpp"

namespace violet
{
struct scan_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/scan.hlsl";
    static constexpr std::string_view entry_point = "scan";

    struct constant_data
    {
        std::uint32_t buffer;
        std::uint32_t size;
        std::uint32_t offset_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct scan_offset_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/scan.hlsl";
    static constexpr std::string_view entry_point = "scan_offset";

    struct constant_data
    {
        std::uint32_t buffer;
        std::uint32_t size;
        std::uint32_t offset_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

void scan_pass::add(render_graph& graph, const parameter& parameter)
{
    assert(parameter.size < group_size * max_group_count);

    if (parameter.size > group_size)
    {
        m_offset_buffer = graph.add_buffer(
            "Scan Offset",
            max_group_count * sizeof(std::uint32_t),
            RHI_BUFFER_STORAGE);
    }

    add_scan_pass(graph, parameter);

    if (parameter.size > group_size)
    {
        add_offset_pass(graph, parameter);
    }
}

void scan_pass::add_scan_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_buffer_uav buffer;
        rdg_buffer_uav offset_buffer;
        std::uint32_t size;
    };

    graph.add_pass<pass_data>(
        "Scan",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.buffer = pass.add_buffer_uav(parameter.buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.size = parameter.size;

            if (parameter.size > group_size)
            {
                data.offset_buffer =
                    pass.add_buffer_uav(m_offset_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            }
            else
            {
                data.offset_buffer.reset();
            }
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            std::vector<std::wstring> defines;
            if (data.size > group_size)
            {
                defines.emplace_back(L"-DOUTPUT_OFFSET");
            }

            command.set_pipeline({
                .compute_shader = device.get_shader<scan_cs>(defines),
            });
            command.set_constant(
                scan_cs::constant_data{
                    .buffer = data.buffer.get_bindless(),
                    .size = data.size,
                    .offset_buffer = data.offset_buffer ? data.offset_buffer.get_bindless() : 0,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(data.size, group_size);
        });
}

void scan_pass::add_offset_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_buffer_uav buffer;
        rdg_buffer_srv offset_buffer;
        std::uint32_t size;
    };

    graph.add_pass<pass_data>(
        "Scan Offset",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.buffer = pass.add_buffer_uav(parameter.buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.offset_buffer = pass.add_buffer_srv(m_offset_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.size = parameter.size;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<scan_offset_cs>(),
            });
            command.set_constant(
                scan_cs::constant_data{
                    .buffer = data.buffer.get_bindless(),
                    .size = data.size,
                    .offset_buffer = data.offset_buffer.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(data.size - group_size, group_size);
        });
}
} // namespace violet