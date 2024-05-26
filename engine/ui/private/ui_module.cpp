#include "ui/ui_module.hpp"
#include "components/camera.hpp"
#include "components/ui_root.hpp"
#include "graphics/graphics_module.hpp"
#include "rendering/ui_render_graph.hpp"
#include "rendering/ui_renderer.hpp"
#include "ui/theme.hpp"
#include "ui/ui_task.hpp"
#include "window/window_module.hpp"
namespace violet
{
ui_module::ui_module() : engine_module("ui")
{
}

bool ui_module::initialize(const dictionary& config)
{
    get_world().register_component<ui_root>();

    m_renderer = std::make_unique<ui_renderer>(get_module<graphics_module>().get_device());

    auto& window = get_module<window_module>();

    load_font("remixicon", "engine/font/remixicon.ttf", 24);
    load_font("NotoSans-Regular", "engine/font/NotoSans-Regular.ttf", 13);

    /*event.subscribe<window::event_keyboard_char>(
        "ui",
        [this](char c)
        {
            m_tree->input(c);
        });*/

    initialize_default_theme();

    on_frame_begin().then(
        []()
        {

        });
    on_tick().then(
        [this](float delta)
        {
            tick();
        });

    return true;
}

void ui_module::tick()
{
    m_renderer->reset();

    view<ui_root, camera> view(get_world());
    view.each(
        [this](ui_root& root, camera& camera)
        {
            rhi_viewport viewport = camera.get_viewport();
            widget* container = root.get_container();
            update_layout(container, viewport.width, viewport.height);
            render(container, root.get_pass());
        });

    /*
     m_renderer->reset();

     auto extent = system<window::window>().extent();
     m_tree->tick(static_cast<float>(extent.width), static_cast<float>(extent.height));

     if (!m_tree->tree_dirty())
         return;

     m_renderer->draw(m_tree.get());*/
}

void ui_module::load_font(std::string_view name, std::string_view ttf_file, std::size_t size)
{
    m_fonts[name.data()] = std::make_unique<font_type>(ttf_file, size);
}

const ui_module::font_type* ui_module::font(std::string_view name)
{
    auto iter = m_fonts.find(name.data());
    if (iter != m_fonts.end())
        return iter->second.get();
    else
        throw std::out_of_range("Font not found.");
}

const ui_module::font_type* ui_module::default_text_font()
{
    return font("NotoSans-Regular");
}

const ui_module::font_type* ui_module::default_icon_font()
{
    return font("remixicon");
}

control* ui_module::root() const noexcept
{
    return nullptr;
    // return m_tree.get();
}

void ui_module::initialize_default_theme()
{
    auto text_font = default_text_font();
    auto icon_font = default_icon_font();

    // Dark.
    {
        std::string theme_name = "dark";
        /*
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
        */
    }
}

void ui_module::update_layout(widget* widget, std::uint32_t width, std::uint32_t height)
{
    layout_node* layout = widget->get_layout();
    layout->calculate(width, height);
}

void ui_module::render(widget* widget, ui_pass* pass)
{
    m_renderer->render(widget, pass);
}
} // namespace violet