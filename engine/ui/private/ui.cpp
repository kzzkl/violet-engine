#include "ui/ui.hpp"
#include "control_tree.hpp"
#include "graphics/graphics_task.hpp"
#include "graphics/mesh_render.hpp"
#include "graphics/rhi.hpp"
#include "render/renderer.hpp"
#include "render/ui_pipeline.hpp"
#include "task/task_manager.hpp"
#include "ui/controls.hpp"
#include "ui/theme.hpp"
#include "ui/ui_task.hpp"
#include "window/window.hpp"
#include "window/window_event.hpp"

namespace violet::ui
{
ui::ui() : system_base("ui")
{
}

bool ui::initialize(const dictionary& config)
{
    m_renderer = std::make_unique<renderer>();

    auto& event = system<core::event>();

    load_font("remixicon", "engine/font/remixicon.ttf", 24);
    load_font("NotoSans-Regular", "engine/font/NotoSans-Regular.ttf", 13);

    m_tree = std::make_unique<control_tree>();

    event.subscribe<window::event_window_resize>(
        "ui",
        [this](std::uint32_t width, std::uint32_t height) { resize(width, height); });
    event.subscribe<window::event_keyboard_char>("ui", [this](char c) { m_tree->input(c); });

    auto window_extent = system<window::window>().extent();
    resize(window_extent.width, window_extent.height);

    auto& task = system<task::task_manager>();
    auto ui_tick_task = task.schedule(TASK_UI_TICK, [this]() { tick(); });
    ui_tick_task->add_dependency(*task.find(task::TASK_GAME_LOGIC_END));

    auto render_task = task.find(graphics::TASK_GRAPHICS_RENDER);
    render_task->add_dependency(*ui_tick_task);

    initialize_default_theme();

    return true;
}

void ui::tick()
{
    m_renderer->reset();

    auto extent = system<window::window>().extent();
    m_tree->tick(static_cast<float>(extent.width), static_cast<float>(extent.height));

    if (!m_tree->tree_dirty())
        return;

    m_renderer->draw(m_tree.get());
}

void ui::load_font(std::string_view name, std::string_view ttf_file, std::size_t size)
{
    m_fonts[name.data()] = std::make_unique<font_type>(ttf_file, size);
}

const ui::font_type* ui::font(std::string_view name)
{
    auto iter = m_fonts.find(name.data());
    if (iter != m_fonts.end())
        return iter->second.get();
    else
        throw std::out_of_range("Font not found.");
}

const ui::font_type* ui::default_text_font()
{
    return font("NotoSans-Regular");
}

const ui::font_type* ui::default_icon_font()
{
    return font("remixicon");
}

control* ui::root() const noexcept
{
    return m_tree.get();
}

void ui::initialize_default_theme()
{
    auto text_font = default_text_font();
    auto icon_font = default_icon_font();

    // Dark.
    {
        std::string theme_name = "dark";

        button_theme button_dark = {
            .text_font = text_font,
            .text_color = COLOR_WHITE,
            .default_color = 0xFF3B291E,
            .highlight_color = 0xFFB2CC38};
        register_theme(theme_name, button_dark);

        icon_button_theme icon_button_dark = {
            .icon_font = icon_font,
            .icon_scale = 1.0f,
            .default_color = 0xFF3B291E,
            .highlight_color = 0xFFB2CC38};
        register_theme(theme_name, icon_button_dark);

        collapse_theme collapse_dark = {
            .icon_font = icon_font,
            .icon_color = COLOR_WHITE,
            .icon_scale = 0.7f,
            .title_font = text_font,
            .title_color = COLOR_WHITE,
            .bar_color = 0xFFB2CC38,
            .container_color = 0xFF5F4B3D};
        register_theme(theme_name, collapse_dark);

        dock_area_theme dock_area_dark = {.hover_color = 0xFF3B291E};
        register_theme(theme_name, dock_area_dark);

        dock_window_theme dock_window_dark = {
            .icon_font = icon_font,
            .icon_color = COLOR_WHITE,
            .icon_scale = 0.7f,
            .title_font = text_font,
            .title_color = COLOR_WHITE,
            .scroll_speed = 30.0f,
            .bar_width = 8.0f,
            .bar_color = 0xFF3B291E,
            .slider_color = 0xFFB2CC38,
            .container_color = 0xFF5F4B3D};
        register_theme(theme_name, dock_window_dark);

        font_icon_theme font_icon_dark = {
            .icon_font = icon_font,
            .icon_color = COLOR_WHITE,
            .icon_scale = 1.0f};
        register_theme(theme_name, font_icon_dark);

        input_theme input_dark = {
            .text_font = text_font,
            .text_color = COLOR_WHITE,
            .background_color = 0xFF3B291E,
            .underline_color = 0xFFB2CC38,
            .select_color = 0xFFB2CC38};
        register_theme(theme_name, input_dark);

        label_theme label_dark = {.text_font = text_font, .text_color = COLOR_WHITE};
        register_theme(theme_name, label_dark);

        scroll_view_theme scroll_view_dark = {
            .scroll_speed = 30.0f,
            .bar_width = 8.0f,
            .bar_color = 0xFF3B291E,
            .slider_color = 0xFFB2CC38,
            .background_color = 0xFF5F4B3D};
        register_theme(theme_name, scroll_view_dark);

        tree_node_theme tree_node_dark = {
            .text_font = text_font,
            .text_color = COLOR_WHITE,
            .icon_font = icon_font,
            .icon_color = COLOR_WHITE,
            .icon_scale = 0.7f,
            .padding_top = 5.0f,
            .padding_bottom = 5.0f,
            .padding_increment = 20.0f,
            .default_color = 0xFF3B291E,
            .highlight_color = 0xFFB2CC38};
        register_theme(theme_name, tree_node_dark);
    }
}

void ui::resize(std::uint32_t width, std::uint32_t height)
{
    m_renderer->resize(width, height);
    m_tree->layout()->set_width(width);
    m_tree->layout()->set_height(height);
}
} // namespace violet::ui