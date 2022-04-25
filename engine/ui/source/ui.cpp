#include "ui.hpp"
#include "graphics.hpp"
#include "imgui.h"
#include "relation.hpp"
#include "window.hpp"
#include "window_event.hpp"

namespace ash::ui
{
namespace
{
int convert_key(window::keyboard_key key)
{
    switch (key)
    {
    case window::keyboard_key::KEY_TAB:
        return ImGuiKey_Tab;
    case window::keyboard_key::KEY_LEFT:
        return ImGuiKey_LeftArrow;
    case window::keyboard_key::KEY_RIGHT:
        return ImGuiKey_RightArrow;
    case window::keyboard_key::KEY_UP:
        return ImGuiKey_UpArrow;
    case window::keyboard_key::KEY_DOWN:
        return ImGuiKey_DownArrow;
    case window::keyboard_key::KEY_PRIOR:
        return ImGuiKey_PageUp;
    case window::keyboard_key::KEY_NEXT:
        return ImGuiKey_PageDown;
    case window::keyboard_key::KEY_HOME:
        return ImGuiKey_Home;
    case window::keyboard_key::KEY_END:
        return ImGuiKey_End;
    case window::keyboard_key::KEY_INSERT:
        return ImGuiKey_Insert;
    case window::keyboard_key::KEY_DELETE:
        return ImGuiKey_Delete;
    case window::keyboard_key::KEY_BACK:
        return ImGuiKey_Backspace;
    case window::keyboard_key::KEY_SPACE:
        return ImGuiKey_Space;
    case window::keyboard_key::KEY_RETURN:
        return ImGuiKey_Enter;
    case window::keyboard_key::KEY_ESCAPE:
        return ImGuiKey_Escape;
    case window::keyboard_key::KEY_OEM_7:
        return ImGuiKey_Apostrophe;
    case window::keyboard_key::KEY_OEM_COMMA:
        return ImGuiKey_Comma;
    case window::keyboard_key::KEY_OEM_MINUS:
        return ImGuiKey_Minus;
    case window::keyboard_key::KEY_OEM_PERIOD:
        return ImGuiKey_Period;
    case window::keyboard_key::KEY_OEM_2:
        return ImGuiKey_Slash;
    case window::keyboard_key::KEY_OEM_1:
        return ImGuiKey_Semicolon;
    case window::keyboard_key::KEY_OEM_PLUS:
        return ImGuiKey_Equal;
    case window::keyboard_key::KEY_OEM_4:
        return ImGuiKey_LeftBracket;
    case window::keyboard_key::KEY_OEM_5:
        return ImGuiKey_Backslash;
    case window::keyboard_key::KEY_OEM_6:
        return ImGuiKey_RightBracket;
    case window::keyboard_key::KEY_OEM_3:
        return ImGuiKey_GraveAccent;
    case window::keyboard_key::KEY_CAPITAL:
        return ImGuiKey_CapsLock;
    case window::keyboard_key::KEY_SCROLL:
        return ImGuiKey_ScrollLock;
    case window::keyboard_key::KEY_NUMLOCK:
        return ImGuiKey_NumLock;
    case window::keyboard_key::KEY_SNAPSHOT:
        return ImGuiKey_PrintScreen;
    case window::keyboard_key::KEY_PAUSE:
        return ImGuiKey_Pause;
    case window::keyboard_key::KEY_NUMPAD0:
        return ImGuiKey_Keypad0;
    case window::keyboard_key::KEY_NUMPAD1:
        return ImGuiKey_Keypad1;
    case window::keyboard_key::KEY_NUMPAD2:
        return ImGuiKey_Keypad2;
    case window::keyboard_key::KEY_NUMPAD3:
        return ImGuiKey_Keypad3;
    case window::keyboard_key::KEY_NUMPAD4:
        return ImGuiKey_Keypad4;
    case window::keyboard_key::KEY_NUMPAD5:
        return ImGuiKey_Keypad5;
    case window::keyboard_key::KEY_NUMPAD6:
        return ImGuiKey_Keypad6;
    case window::keyboard_key::KEY_NUMPAD7:
        return ImGuiKey_Keypad7;
    case window::keyboard_key::KEY_NUMPAD8:
        return ImGuiKey_Keypad8;
    case window::keyboard_key::KEY_NUMPAD9:
        return ImGuiKey_Keypad9;
    case window::keyboard_key::KEY_DECIMAL:
        return ImGuiKey_KeypadDecimal;
    case window::keyboard_key::KEY_DIVIDE:
        return ImGuiKey_KeypadDivide;
    case window::keyboard_key::KEY_MULTIPLY:
        return ImGuiKey_KeypadMultiply;
    case window::keyboard_key::KEY_SUBTRACT:
        return ImGuiKey_KeypadSubtract;
    case window::keyboard_key::KEY_ADD:
        return ImGuiKey_KeypadAdd;
    case window::keyboard_key::KEY_LSHIFT:
        return ImGuiKey_LeftShift;
    case window::keyboard_key::KEY_LCONTROL:
        return ImGuiKey_LeftCtrl;
    case window::keyboard_key::KEY_LMENU:
        return ImGuiKey_LeftAlt;
    case window::keyboard_key::KEY_LWIN:
        return ImGuiKey_LeftSuper;
    case window::keyboard_key::KEY_RSHIFT:
        return ImGuiKey_RightShift;
    case window::keyboard_key::KEY_RCONTROL:
        return ImGuiKey_RightCtrl;
    case window::keyboard_key::KEY_RMENU:
        return ImGuiKey_RightAlt;
    case window::keyboard_key::KEY_RWIN:
        return ImGuiKey_RightSuper;
    case window::keyboard_key::KEY_APPS:
        return ImGuiKey_Menu;
    case window::keyboard_key::KEY_0:
        return ImGuiKey_0;
    case window::keyboard_key::KEY_1:
        return ImGuiKey_1;
    case window::keyboard_key::KEY_2:
        return ImGuiKey_2;
    case window::keyboard_key::KEY_3:
        return ImGuiKey_3;
    case window::keyboard_key::KEY_4:
        return ImGuiKey_4;
    case window::keyboard_key::KEY_5:
        return ImGuiKey_5;
    case window::keyboard_key::KEY_6:
        return ImGuiKey_6;
    case window::keyboard_key::KEY_7:
        return ImGuiKey_7;
    case window::keyboard_key::KEY_8:
        return ImGuiKey_8;
    case window::keyboard_key::KEY_9:
        return ImGuiKey_9;
    case window::keyboard_key::KEY_A:
        return ImGuiKey_A;
    case window::keyboard_key::KEY_B:
        return ImGuiKey_B;
    case window::keyboard_key::KEY_C:
        return ImGuiKey_C;
    case window::keyboard_key::KEY_D:
        return ImGuiKey_D;
    case window::keyboard_key::KEY_E:
        return ImGuiKey_E;
    case window::keyboard_key::KEY_F:
        return ImGuiKey_F;
    case window::keyboard_key::KEY_G:
        return ImGuiKey_G;
    case window::keyboard_key::KEY_H:
        return ImGuiKey_H;
    case window::keyboard_key::KEY_I:
        return ImGuiKey_I;
    case window::keyboard_key::KEY_J:
        return ImGuiKey_J;
    case window::keyboard_key::KEY_K:
        return ImGuiKey_K;
    case window::keyboard_key::KEY_L:
        return ImGuiKey_L;
    case window::keyboard_key::KEY_M:
        return ImGuiKey_M;
    case window::keyboard_key::KEY_N:
        return ImGuiKey_N;
    case window::keyboard_key::KEY_O:
        return ImGuiKey_O;
    case window::keyboard_key::KEY_P:
        return ImGuiKey_P;
    case window::keyboard_key::KEY_Q:
        return ImGuiKey_Q;
    case window::keyboard_key::KEY_R:
        return ImGuiKey_R;
    case window::keyboard_key::KEY_S:
        return ImGuiKey_S;
    case window::keyboard_key::KEY_T:
        return ImGuiKey_T;
    case window::keyboard_key::KEY_U:
        return ImGuiKey_U;
    case window::keyboard_key::KEY_V:
        return ImGuiKey_V;
    case window::keyboard_key::KEY_W:
        return ImGuiKey_W;
    case window::keyboard_key::KEY_X:
        return ImGuiKey_X;
    case window::keyboard_key::KEY_Y:
        return ImGuiKey_Y;
    case window::keyboard_key::KEY_Z:
        return ImGuiKey_Z;
    case window::keyboard_key::KEY_F1:
        return ImGuiKey_F1;
    case window::keyboard_key::KEY_F2:
        return ImGuiKey_F2;
    case window::keyboard_key::KEY_F3:
        return ImGuiKey_F3;
    case window::keyboard_key::KEY_F4:
        return ImGuiKey_F4;
    case window::keyboard_key::KEY_F5:
        return ImGuiKey_F5;
    case window::keyboard_key::KEY_F6:
        return ImGuiKey_F6;
    case window::keyboard_key::KEY_F7:
        return ImGuiKey_F7;
    case window::keyboard_key::KEY_F8:
        return ImGuiKey_F8;
    case window::keyboard_key::KEY_F9:
        return ImGuiKey_F9;
    case window::keyboard_key::KEY_F10:
        return ImGuiKey_F10;
    case window::keyboard_key::KEY_F11:
        return ImGuiKey_F11;
    case window::keyboard_key::KEY_F12:
        return ImGuiKey_F12;
    default:
        return ImGuiKey_None;
    }
};
} // namespace

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
    world.component<graphics::visual>(m_ui_entity).mask = (1 << 1);

    ImGui::CreateContext();
    initialize_theme();
    initialize_font_texture();

    for (std::size_t i = 0; i < 3; ++i)
    {
        m_vertex_buffer.push_back(
            graphics.make_vertex_buffer<ImDrawVert>(nullptr, 1024 * 16, true));
        m_index_buffer.push_back(graphics.make_index_buffer<ImDrawIdx>(nullptr, 1024 * 32, true));
    }

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

    event.subscribe<window::event_keyboard_key>(
        [](window::keyboard_key key, window::key_state state) {
            ImGui::GetIO().AddKeyEvent(convert_key(key), state.down());
        });
    event.subscribe<window::event_keyboard_char>(
        [](char c) { ImGui::GetIO().AddInputCharacter(c); });
    event.subscribe<window::event_window_resize>([](std::uint32_t width, std::uint32_t height) {});

    return true;
}

void ui::window(std::string_view label)
{
    ImGui::Begin(label.data());
}

bool ui::window_ex(std::string_view label)
{
    window(label);
    return ImGui::IsWindowFocused();
}

void ui::window_root(std::string_view label)
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::Begin(
        label.data(),
        nullptr,
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove);
}

void ui::window_pop()
{
    ImGui::End();
}

void ui::text(std::string_view text)
{
    ImGui::Text(text.data());
}

bool ui::tree(std::string_view label, bool leaf)
{
    static constexpr ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                                     ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                     ImGuiTreeNodeFlags_SpanAvailWidth;

    static constexpr ImGuiTreeNodeFlags leaf_flags = base_flags | ImGuiTreeNodeFlags_Leaf;

    return ImGui::TreeNodeEx(label.data(), leaf ? leaf_flags : base_flags);
}

std::tuple<bool, bool> ui::tree_ex(std::string_view label, bool leaf)
{
    bool open = tree(label, leaf);
    bool clicked = ImGui::IsItemClicked();
    return {open, clicked};
}

void ui::tree_pop()
{
    ImGui::TreePop();
}

bool ui::collapsing(std::string_view label)
{
    return ImGui::CollapsingHeader(label.data());
}

void ui::texture(graphics::resource* texture, float width, float height)
{
    ImVec2 pos_min = ImGui::GetCursorScreenPos();
    ImVec2 pos_max = ImVec2(pos_min.x + width, pos_min.y + height);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddImage(texture, pos_min, pos_max);
}

bool ui::drag(std::string_view label, float& value, float speed)
{
    return ImGui::DragFloat(label.data(), &value, speed);
}

bool ui::drag(std::string_view label, math::float3& value, float speed)
{
    return ImGui::DragFloat3(label.data(), &value[0], speed);
}

void ui::style(ui_style style, float x, float y)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(x, y));
}

void ui::style_pop()
{
    ImGui::PopStyleVar();
}

std::pair<std::uint32_t, std::uint32_t> ui::window_size()
{
    auto size = ImGui::GetContentRegionAvail();
    return {static_cast<std::uint32_t>(size.x), static_cast<std::uint32_t>(size.y)};
}

bool ui::any_item_active() const
{
    return ImGui::IsAnyItemActive();
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

    m_parameter_counter = 0;

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
            submesh.index_start = command.IdxOffset + index_counter;
            submesh.index_end = command.IdxOffset + command.ElemCount + index_counter;
            submesh.vertex_base = command.VtxOffset + vertex_counter;
            submesh.pipeline = m_pipeline.get();

            auto parameter = allocate_parameter();
            parameter->set(0, mvp, false);
            parameter->set(1, static_cast<graphics::resource*>(command.GetTexID()));
            // parameter->set(1, m_font.get());
            submesh.parameters.push_back(parameter);

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
}

void ui::initialize_theme()
{
    auto& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("engine/font/Ruda-Bold.ttf", 12 * 1.5);
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigDockingAlwaysTabBar = true;

    auto& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
    style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.30f, 0.31f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.09f, 0.83f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.33f, 0.34f, 0.36f, 0.83f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.23f, 0.24f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    // style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    // style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    style.GrabRounding = style.FrameRounding = 2.3f;
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

graphics::render_parameter* ui::allocate_parameter()
{
    if (m_parameter_counter >= m_parameter_pool.size())
        m_parameter_pool.push_back(system<graphics::graphics>().make_render_parameter("ui"));

    ++m_parameter_counter;
    return m_parameter_pool[m_parameter_counter - 1].get();
}
} // namespace ash::ui