#include "ui_impl_imgui.hpp"
#include "window_event.hpp"

namespace ash::ui
{
ui_impl_imgui::ui_impl_imgui(const ui_impl_desc& desc) : m_index(0)
{
    ImGui::CreateContext();
    // ImGui::GetMainViewport()->PlatformHandleRaw = desc.window_handle;

    for (std::size_t i = 0; i < 3; ++i)
    {
        m_vertex_buffer.push_back(
            desc.graphics->make_vertex_buffer<ImDrawVert>(nullptr, 1024 * 16, true));
        m_index_buffer.push_back(
            desc.graphics->make_index_buffer<ImDrawIdx>(nullptr, 1024 * 32, true));
    }

    m_parameter = desc.graphics->make_render_parameter("ui");

    unsigned char* pixels;
    int font_width, font_height;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &font_width, &font_height);

    m_font = desc.graphics->make_texture(pixels, font_width, font_height);

    desc.event->subscribe<window::event_mouse_key>(
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

    desc.event->subscribe<window::event_mouse_move>([this](window::mouse_mode mode, int x, int y) {
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
}

void ui_impl_imgui::begin_frame(std::uint32_t width, std::uint32_t height, float delta)
{
    m_index = (m_index + 1) % 3;
    m_data.clear();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    io.DeltaTime = delta;

    ImGui::NewFrame();
}

void ui_impl_imgui::end_frame()
{
    ImGui::Render();

    auto data = ImGui::GetDrawData();

    std::size_t vertex_counter = 0;
    std::size_t index_counter = 0;
    std::size_t vertex_buffer_offset = 0;
    std::size_t index_buffer_offset = 0;
    for (int i = 0; i < data->CmdListsCount; ++i)
    {
        auto list = data->CmdLists[i];
        m_vertex_buffer[m_index]->upload(
            list->VtxBuffer.Data,
            list->VtxBuffer.Size * sizeof(ImDrawVert),
            vertex_buffer_offset);
        vertex_buffer_offset += list->VtxBuffer.Size * sizeof(ImDrawVert);

        m_index_buffer[m_index]->upload(
            list->IdxBuffer.Data,
            list->IdxBuffer.Size * sizeof(ImDrawIdx),
            index_buffer_offset);
        index_buffer_offset += list->IdxBuffer.Size * sizeof(ImDrawIdx);

        for (int j = 0; j < list->CmdBuffer.Size; ++j)
        {
            auto& command = list->CmdBuffer[j];

            graphics::render_unit submesh = {};
            submesh.vertex_buffer = m_vertex_buffer[m_index].get();
            submesh.index_buffer = m_index_buffer[m_index].get();
            submesh.parameters.push_back(m_parameter.get());
            submesh.index_start = command.IdxOffset + index_counter;
            submesh.index_end = command.IdxOffset + command.ElemCount + index_counter;
            submesh.vertex_base = command.VtxOffset + vertex_counter;

            m_data.push_back(submesh);
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

void ui_impl_imgui::test()
{
    bool show = true;
    ImGui::ShowDemoWindow(&show);
    ImGui::Begin("Hello, world!");
    ImGui::Text("This is some useful text.");
    ImGui::Button("Button");

    ImGui::End();
}
} // namespace ash::ui