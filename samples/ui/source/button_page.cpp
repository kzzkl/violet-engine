#include "button_page.hpp"
#include "ui/ui.hpp"

namespace ash::sample
{
button_page::button_page() : m_click_counter(0)
{
    m_title = std::make_unique<text_title_1>("Button");
    m_title->link(this);

    m_description = std::make_unique<text_content>("Commonly used buttons.");
    m_description->link(this);

    for (std::size_t i = 0; i < 2; ++i)
    {
        auto display = std::make_unique<display_panel>();
        display->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);
        display->align_items(ui::LAYOUT_ALIGN_CENTER);
        m_display.push_back(std::move(display));
    }
    initialize_sample_button();
}

void button_page::initialize_sample_button()
{
    auto& text_font = system<ui::ui>().font(ui::DEFAULT_TEXT_FONT);
    auto& icon_font = system<ui::ui>().font(ui::DEFAULT_ICON_FONT);

    // Simple button.
    m_button_title = std::make_unique<text_title_2>("Simple button");
    m_button_title->link(this);
    m_display[0]->link(this);

    m_button = std::make_unique<ui::button>("Default", text_font);
    m_button->width(150.0f);
    m_button->height(40.0f);
    m_button->on_mouse_press = [&, this](window::mouse_key key, int x, int y) -> bool {
        ++m_click_counter;
        m_button_text->text("click: " + std::to_string(m_click_counter), text_font);
        return false;
    };
    m_button->link(m_display[0].get());

    m_button_text = std::make_unique<ui::label>("click: 0", text_font);
    m_button_text->margin(50.0f, ui::LAYOUT_EDGE_LEFT);
    m_button_text->link(m_display[0].get());

    // Icon button.
    m_icon_button_title = std::make_unique<text_title_2>("Icon button");
    m_icon_button_title->link(this);
    m_display[1]->link(this);

    m_icon_button = std::make_unique<ui::icon_button>(0xEA22, icon_font);
    m_icon_button->on_mouse_press = [&, this](window::mouse_key key, int x, int y) -> bool {
        m_icon_button_text->text("Don't click me!", text_font);
        return false;
    };
    m_icon_button->link(m_display[1].get());

    m_icon_button_text = std::make_unique<ui::label>("hello!", text_font);
    m_icon_button_text->margin(50.0f, ui::LAYOUT_EDGE_LEFT);
    m_icon_button_text->link(m_display[1].get());
}
} // namespace ash::sample