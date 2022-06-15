#include "button_page.hpp"
#include "ui/ui.hpp"

namespace ash::sample
{
button_page::button_page() : page("Button"), m_click_counter(0)
{
    add_description("Commonly used buttons.");
    initialize_sample_button();
}

void button_page::initialize_sample_button()
{
    auto& ui = system<ui::ui>();

    // Simple button.
    add_subtitle("Simple button");

    auto display_1 = add_display_panel();
    display_1->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);
    display_1->align_items(ui::LAYOUT_ALIGN_CENTER);

    m_button = std::make_unique<ui::button>("Default", ui.theme<ui::button_theme>("dark"));
    m_button->width(150.0f);
    m_button->height(40.0f);
    m_button->on_mouse_press = [&, this](window::mouse_key key, int x, int y) -> bool {
        ++m_click_counter;
        m_button_text->text("click: " + std::to_string(m_click_counter));
        return false;
    };
    m_button->link(display_1);

    m_button_text = std::make_unique<ui::label>("click: 0", ui.theme<ui::label_theme>("dark"));
    m_button_text->margin(50.0f, ui::LAYOUT_EDGE_LEFT);
    m_button_text->link(display_1);

    // Icon button.
    add_subtitle("Icon button");

    auto display_2 = add_display_panel();
    display_2->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);
    display_2->align_items(ui::LAYOUT_ALIGN_CENTER);

    m_icon_button =
        std::make_unique<ui::icon_button>(0xEA22, ui.theme<ui::icon_button_theme>("dark"));
    m_icon_button->on_mouse_press = [&, this](window::mouse_key key, int x, int y) -> bool {
        m_icon_button_text->text("Don't click me!");
        return false;
    };
    m_icon_button->link(display_2);

    m_icon_button_text = std::make_unique<ui::label>("hello!", ui.theme<ui::label_theme>("dark"));
    m_icon_button_text->margin(50.0f, ui::LAYOUT_EDGE_LEFT);
    m_icon_button_text->link(display_2);
}
} // namespace ash::sample