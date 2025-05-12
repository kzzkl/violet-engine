#include "graphics/passes/taa_pass.hpp"

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
        {0, bindless},
        {1, camera},
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
                data.history_render_target = rdg_texture_srv();
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
                data.motion_vector = rdg_texture_srv();
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

void taa_render_feature::on_update(std::uint32_t width, std::uint32_t height)
{
    bool recreate = false;

    if (m_history[0] == nullptr)
    {
        recreate = true;
    }
    else
    {
        auto extent = m_history[0]->get_extent();
        recreate = extent.width != width || extent.height != height;
    }

    if (recreate)
    {
        for (auto& target : m_history)
        {
            target = render_device::instance().create_texture({
                .extent = {width, height},
                .format = RHI_FORMAT_R16G16B16A16_FLOAT,
                .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE,
                .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            });
        }

        m_frame = 0;
    }
    else
    {
        ++m_frame;
    }
}
} // namespace violet