#include "imgui_system.hpp"
#include "graphics/tools/texture_loader.hpp"
#include "imgui.h"
#include "window/window_system.hpp"

namespace violet
{
imgui_system::imgui_system()
    : engine_system("imgui")
{
}

bool imgui_system::initialize(const dictionary& config)
{
    auto& world = get_world();

    ImGui::CreateContext();
    initialize_font();

    auto& task_graph = get_task_graph();
    auto& pre_update_group = task_graph.get_group("PreUpdate");
    auto& post_update_group = task_graph.get_group("PostUpdate");
    auto& update_window_task = task_graph.get_task("Update Window");
    auto& rendering_group = task_graph.get_group("Rendering");

    task_graph.add_task()
        .set_name("ImGui Begin")
        .set_group(pre_update_group)
        .add_dependency(update_window_task)
        .set_execute(
            [this]()
            {
                begin_frame();
            });

    auto& end_task = task_graph.add_task()
                         .set_name("ImGui End")
                         .set_group(post_update_group)
                         .set_execute(
                             [this]()
                             {
                                 end_frame();
                             });

    rendering_group.add_dependency(end_task);

    return true;
}

void imgui_system::initialize_font()
{
    auto& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("assets/fonts/NotoSans-SemiBold.ttf", 12 * 1.5);

    unsigned char* pixels;
    int font_width;
    int font_height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &font_width, &font_height);

    texture_loader::mipmap_data font_mipmap_data;
    font_mipmap_data.extent.width = font_width;
    font_mipmap_data.extent.height = font_height;
    font_mipmap_data.pixels.resize(4ull * font_width * font_height);
    std::memcpy(font_mipmap_data.pixels.data(), pixels, font_mipmap_data.pixels.size());

    texture_loader::texture_data font_texture_data;
    font_texture_data.format = RHI_FORMAT_R8G8B8A8_UNORM;
    font_texture_data.mipmaps.push_back(font_mipmap_data);

    m_font = texture_loader::load(font_texture_data);

    io.Fonts->SetTexID(m_font->get_handle());
}

void imgui_system::begin_frame()
{
    auto& window = get_system<window_system>();

    auto window_extent = window.get_extent();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize =
        ImVec2(static_cast<float>(window_extent.width), static_cast<float>(window_extent.height));
    io.DeltaTime = get_timer().get_frame_delta();
    ImGui::NewFrame();

    ImGui::GetIO().AddMouseButtonEvent(
        ImGuiMouseButton_Left,
        window.get_mouse().key(MOUSE_KEY_LEFT).down());
    ImGui::GetIO().AddMouseButtonEvent(
        ImGuiMouseButton_Right,
        window.get_mouse().key(MOUSE_KEY_RIGHT).down());
    ImGui::GetIO().AddMouseButtonEvent(
        ImGuiMouseButton_Middle,
        window.get_mouse().key(MOUSE_KEY_MIDDLE).down());

    ImGui::GetIO().AddMousePosEvent(
        static_cast<float>(window.get_mouse().get_x()),
        static_cast<float>(window.get_mouse().get_y()));
}

void imgui_system::end_frame()
{
    ImGui::Render();
}
} // namespace violet