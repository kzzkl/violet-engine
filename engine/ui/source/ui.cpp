#include "ui.hpp"
#include "graphics.hpp"
#include "imgui.h"
#include "relation.hpp"
#include "window.hpp"
#include "window_event.hpp"

namespace ash::ui
{
ui::ui() : core::system_base("ui")
{
}

bool ui::initialize(const dictionary& config)
{
    auto& graphics = system<graphics::graphics>();
    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    m_pipeline = graphics.make_render_pipeline<ui_pipeline>("ui");

    m_ui_entity = world.create();
    world.add<graphics::visual>(m_ui_entity);

    ImGui::CreateContext();
    initialize_theme();
    initialize_font_texture();

    for (std::size_t i = 0; i < 3; ++i)
    {
        m_vertex_buffer.push_back(
            graphics.make_vertex_buffer<ImDrawVert>(nullptr, 1024 * 16, true));
        m_index_buffer.push_back(graphics.make_index_buffer<ImDrawIdx>(nullptr, 1024 * 32, true));
    }

    m_parameter = graphics.make_render_parameter("ui");

    event.subscribe<window::event_mouse_key>(
        [this](window::mouse_key key, window::key_state state) {
            if (m_enable_mouse)
            {
                int button = 0;
                if (key == window::mouse_key::LEFT_BUTTON)
                    button = 0;
                else if (key == window::mouse_key::RIGHT_BUTTON)
                    button = 0;
                else if (key == window::mouse_key::MIDDLE_BUTTON)
                    button = 0;

                ImGui::GetIO().AddMouseButtonEvent(button, state.down());
            }
        });

    event.subscribe<window::event_mouse_move>([this](window::mouse_mode mode, int x, int y) {
        if (mode == window::mouse_mode::CURSOR_ABSOLUTE)
        {
            m_enable_mouse = true;
            ImGui::GetIO().AddMousePosEvent(static_cast<float>(x), static_cast<float>(y));
        }
        else if (m_enable_mouse)
        {
            m_enable_mouse = false;
            ImGui::GetIO().AddMousePosEvent(0.0f, 0.0f);
        }
    });

    return true;
}

void ui::begin_frame()
{
    auto& window = system<window::window>();

    window::window_rect rect = window.rect();

    static auto old_time = system<core::timer>().now();
    auto new_time = system<core::timer>().now();
    float delta = (new_time - old_time).count() * 0.000000001f;
    old_time = new_time;

    m_frame_index = (m_frame_index + 1) % 3;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(rect.width), static_cast<float>(rect.height));
    io.DeltaTime = delta;

    ImGui::NewFrame();
}

void ui::end_frame()
{
    bool show = true;
    ImGui::ShowDemoWindow(&show);
    ImGui::Begin("Hello, world!");
    ImGui::Text("This is some useful text.");
    ImGui::Button("Button");

    ImGui::End();

    auto& world = system<ecs::world>();
    auto& visual = world.component<graphics::visual>(m_ui_entity);
    visual.submesh.clear();
    m_scissor_rects.clear();

    ImGui::Render();

    auto data = ImGui::GetDrawData();

    std::size_t vertex_counter = 0;
    std::size_t index_counter = 0;
    std::size_t vertex_buffer_offset = 0;
    std::size_t index_buffer_offset = 0;

    ImVec2 clip_off = data->DisplayPos;
    for (int i = 0; i < data->CmdListsCount; ++i)
    {
        auto list = data->CmdLists[i];
        m_vertex_buffer[m_frame_index]->upload(
            list->VtxBuffer.Data,
            list->VtxBuffer.Size * sizeof(ImDrawVert),
            vertex_buffer_offset);
        vertex_buffer_offset += list->VtxBuffer.Size * sizeof(ImDrawVert);

        m_index_buffer[m_frame_index]->upload(
            list->IdxBuffer.Data,
            list->IdxBuffer.Size * sizeof(ImDrawIdx),
            index_buffer_offset);
        index_buffer_offset += list->IdxBuffer.Size * sizeof(ImDrawIdx);

        for (int j = 0; j < list->CmdBuffer.Size; ++j)
        {
            auto& command = list->CmdBuffer[j];

            graphics::render_unit submesh = {};
            submesh.vertex_buffer = m_vertex_buffer[m_frame_index].get();
            submesh.index_buffer = m_index_buffer[m_frame_index].get();
            submesh.parameters.push_back(m_parameter.get());
            submesh.index_start = command.IdxOffset + index_counter;
            submesh.index_end = command.IdxOffset + command.ElemCount + index_counter;
            submesh.vertex_base = command.VtxOffset + vertex_counter;
            submesh.pipeline = m_pipeline.get();

            ImVec2 clip_min(command.ClipRect.x - clip_off.x, command.ClipRect.y - clip_off.y);
            ImVec2 clip_max(command.ClipRect.z - clip_off.x, command.ClipRect.w - clip_off.y);
            if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                continue;

            // Apply Scissor/clipping rectangle
            graphics::scissor_rect rect = {
                static_cast<std::uint32_t>(clip_min.x),
                static_cast<std::uint32_t>(clip_min.y),
                static_cast<std::uint32_t>(clip_max.x),
                static_cast<std::uint32_t>(clip_max.y)};
            m_scissor_rects.push_back(rect);
            submesh.external = &m_scissor_rects.back();

            visual.submesh.push_back(submesh);
        }

        vertex_counter += list->VtxBuffer.Size;
        index_counter += list->IdxBuffer.Size;
    }

    float L = data->DisplayPos.x;
    float R = data->DisplayPos.x + data->DisplaySize.x;
    float T = data->DisplayPos.y;
    float B = data->DisplayPos.y + data->DisplaySize.y;

    math::float4x4 mvp = {
        math::float4{2.0f / (R - L),    0.0f,              0.0f, 0.0f},
        math::float4{0.0f,              2.0f / (T - B),    0.0f, 0.0f},
        math::float4{0.0f,              0.0f,              0.5f, 0.0f},
        math::float4{(R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f}
    };
    m_parameter->set(0, mvp, false);
    m_parameter->set(1, m_font.get());
}

void ui::initialize_theme()
{
    auto& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("engine/font/Ruda-Bold.ttf", 12 * 1.5);
}

void ui::initialize_font_texture()
{
    auto& graphics = system<graphics::graphics>();
    auto& io = ImGui::GetIO();

    unsigned char* pixels;
    int font_width, font_height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &font_width, &font_height);
    m_font = graphics.make_texture(pixels, font_width, font_height);

    io.Fonts->SetTexID(m_font.get());
}
} // namespace ash::ui