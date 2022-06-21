#include "input_page.hpp"
#include "ui/ui.hpp"
#include <format>

namespace ash::sample
{
input_page::input_page() : page("Input")
{
    add_description("You can enter characters into the input control through the keyboard.");

    initialize_input_text();
    initialize_input_float();
}

void input_page::initialize_input_text()
{
    auto& ui = system<ui::ui>();

    add_subtitle("Text input");

    auto display = add_display_panel();
    display->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);
    display->align_items(ui::LAYOUT_ALIGN_CENTER);

    m_input_text = std::make_unique<ui::input>(ui.theme<ui::input_theme>("dark"));
    m_input_text->width(200.0f);
    m_input_text->on_text_change = [this](std::string_view text) {
        if (text.empty())
            m_input_text_result->text(" ");
        else
            m_input_text_result->text(text);
    };
    display->add(m_input_text.get());

    m_input_text_result = std::make_unique<ui::label>(" ", ui.theme<ui::label_theme>("dark"));
    m_input_text_result->margin(200.0f, ui::LAYOUT_EDGE_LEFT);
    display->add(m_input_text_result.get());
}

void input_page::initialize_input_float()
{
    auto& ui = system<ui::ui>();

    add_subtitle("Float input");

    auto display = add_display_panel();
    display->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);
    display->align_items(ui::LAYOUT_ALIGN_CENTER);

    m_input_float = std::make_unique<ui::input_float>(ui.theme<ui::input_theme>("dark"));
    m_input_float->width(200.0f);
    m_input_float->on_value_change = [this](float value) {
        m_input_float_result->text(std::format("{}", value));
    };
    display->add(m_input_float.get());

    m_input_float_result = std::make_unique<ui::label>(" ", ui.theme<ui::label_theme>("dark"));
    m_input_float_result->margin(200.0f, ui::LAYOUT_EDGE_LEFT);
    display->add(m_input_float_result.get());
}
} // namespace ash::sample