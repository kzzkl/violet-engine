#include "imgui_pass.hpp"
#include "imgui.h"

namespace violet
{
struct imgui_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/imgui.hlsl";

    static constexpr input_layout inputs = {
        {"position", RHI_FORMAT_R32G32_FLOAT},
        {"texcoord", RHI_FORMAT_R32G32_FLOAT},
        {"color", RHI_FORMAT_R32_UINT},
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_VERTEX | RHI_SHADER_STAGE_FRAGMENT,
            .size = sizeof(imgui_pass::draw_constant),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, parameter},
    };
};

struct imgui_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/imgui.hlsl";

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, imgui_vs::parameter},
    };
};

imgui_pass::imgui_pass()
{
    static constexpr std::size_t max_vertex_count = 16ull * 1024;
    static constexpr std::size_t max_index_count = 32ull * 1024;

    auto& device = render_device::instance();
    m_geometries.resize(device.get_frame_resource_count());

    for (auto& geometry : m_geometries)
    {
        geometry.position = device.create_buffer({
            .size = sizeof(vec2f) * max_vertex_count,
            .flags = RHI_BUFFER_VERTEX | RHI_BUFFER_HOST_VISIBLE,
        });
        geometry.texcoord = device.create_buffer({
            .size = sizeof(vec2f) * max_vertex_count,
            .flags = RHI_BUFFER_VERTEX | RHI_BUFFER_HOST_VISIBLE,
        });
        geometry.color = device.create_buffer({
            .size = sizeof(std::uint32_t) * max_vertex_count,
            .flags = RHI_BUFFER_VERTEX | RHI_BUFFER_HOST_VISIBLE,
        });
        geometry.index = device.create_buffer({
            .size = sizeof(std::uint32_t) * max_index_count,
            .flags = RHI_BUFFER_INDEX | RHI_BUFFER_HOST_VISIBLE,
        });
    }
}

void imgui_pass::add(render_graph& graph, const parameter& parameter)
{
    update_vertex();

    struct pass_data
    {
        rhi_parameter* material_parameter;
    };

    graph.add_pass<pass_data>(
        "ImGUI",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            pass.add_render_target(parameter.render_target, RHI_ATTACHMENT_LOAD_OP_LOAD);

            data.material_parameter = pass.add_parameter(imgui_vs::parameter);
            data.material_parameter->set_constant(0, &m_constant, sizeof(draw_constant));
        },
        [this](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();
            auto& geometry = m_geometries[device.get_frame_resource_index()];

            rdg_raster_pipeline pipeline = {
                .vertex_shader = device.get_shader<imgui_vs>(),
                .fragment_shader = device.get_shader<imgui_fs>(),
            };

            rhi_attachment_blend& blend = pipeline.blend.attachments[0];
            blend.enable = true;
            blend.src_color_factor = RHI_BLEND_FACTOR_SOURCE_ALPHA;
            blend.dst_color_factor = RHI_BLEND_FACTOR_SOURCE_INV_ALPHA;
            blend.color_op = RHI_BLEND_OP_ADD;
            blend.src_alpha_factor = RHI_BLEND_FACTOR_ONE;
            blend.dst_alpha_factor = RHI_BLEND_FACTOR_SOURCE_INV_ALPHA;
            blend.alpha_op = RHI_BLEND_OP_ADD;

            command.set_pipeline(pipeline);
            command.set_parameter(0, device.get_bindless_parameter());
            command.set_parameter(1, data.material_parameter);

            std::vector<rhi_buffer*> vertex_buffers = {
                geometry.position.get(),
                geometry.texcoord.get(),
                geometry.color.get(),
            };
            command.set_vertex_buffers(vertex_buffers);
            command.set_index_buffer(geometry.index.get(), sizeof(std::uint32_t));

            for (std::size_t i = 0; i < m_draw_calls.size(); ++i)
            {
                const auto& draw_call = m_draw_calls[i];

                command.set_scissor(std::span(&draw_call.scissor, 1));
                command.draw_indexed(
                    draw_call.index_offset,
                    draw_call.index_count,
                    draw_call.vertex_offset,
                    static_cast<std::uint32_t>(i),
                    1);
            }
        });
}

void imgui_pass::update_vertex()
{
    auto& device = render_device::instance();
    auto& geometry = m_geometries[device.get_frame_resource_index()];

    const auto* data = ImGui::GetDrawData();

    std::size_t vertex_counter = 0;
    std::size_t index_counter = 0;

    m_draw_calls.clear();

    float l = data->DisplayPos.x;
    float r = data->DisplayPos.x + data->DisplaySize.x;
    float t = data->DisplayPos.y;
    float b = data->DisplayPos.y + data->DisplaySize.y;

    m_constant.mvp = {
        vec4f{2.0f / (r - l), 0.0f, 0.0f, 0.0f},
        vec4f{0.0f, 2.0f / (t - b), 0.0f, 0.0f},
        vec4f{0.0f, 0.0f, 0.5f, 0.0f},
        vec4f{(r + l) / (l - r), (t + b) / (b - t), 0.5f, 1.0f}};

    ImVec2 clip_off = data->DisplayPos;

    auto* position = static_cast<vec2f*>(geometry.position->get_buffer_pointer());
    auto* texcoord = static_cast<vec2f*>(geometry.texcoord->get_buffer_pointer());
    auto* color = static_cast<std::uint32_t*>(geometry.color->get_buffer_pointer());
    auto* index = static_cast<std::uint32_t*>(geometry.index->get_buffer_pointer());

    for (int i = 0; i < data->CmdListsCount; ++i)
    {
        const auto* list = data->CmdLists[i];

        for (int j = 0; j < list->VtxBuffer.Size; ++j)
        {
            *position = {
                list->VtxBuffer[j].pos.x,
                list->VtxBuffer[j].pos.y,
            };
            ++position;

            *texcoord = {
                list->VtxBuffer[j].uv.x,
                list->VtxBuffer[j].uv.y,
            };
            ++texcoord;

            *color = list->VtxBuffer[j].col;
            ++color;
        }

        for (int j = 0; j < list->IdxBuffer.Size; ++j)
        {
            *index = list->IdxBuffer[j];
            ++index;
        }

        for (int j = 0; j < list->CmdBuffer.Size; ++j)
        {
            const auto& command = list->CmdBuffer[j];

            draw_call draw_call = {
                .vertex_offset = static_cast<std::uint32_t>(command.VtxOffset + vertex_counter),
                .index_offset = static_cast<std::uint32_t>(command.IdxOffset + index_counter),
                .index_count = command.ElemCount,
            };

            ImVec2 clip_min(command.ClipRect.x - clip_off.x, command.ClipRect.y - clip_off.y);
            ImVec2 clip_max(command.ClipRect.z - clip_off.x, command.ClipRect.w - clip_off.y);
            if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
            {
                continue;
            }

            if (m_draw_calls.size() > 64)
            {
                break;
            }

            m_constant.textures[m_draw_calls.size()] =
                static_cast<std::uint32_t>(command.GetTexID());

            // Apply Scissor/clipping rectangle
            draw_call.scissor = {
                static_cast<std::uint32_t>(clip_min.x),
                static_cast<std::uint32_t>(clip_min.y),
                static_cast<std::uint32_t>(clip_max.x),
                static_cast<std::uint32_t>(clip_max.y)};
            m_draw_calls.push_back(draw_call);
        }

        vertex_counter += list->VtxBuffer.Size;
        index_counter += list->IdxBuffer.Size;
    }
}
} // namespace violet