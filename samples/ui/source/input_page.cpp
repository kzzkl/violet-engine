#include "input_page.hpp"
#include "ui/ui.hpp"

namespace ash::sample
{
input_page::input_page() : page("Input")
{
    add_description("You can enter characters into the input control through the keyboard.");

    initialize_sample_input();
}

void input_page::initialize_sample_input()
{
    auto& ui = system<ui::ui>();

    auto display_1 = add_display_panel();
    display_1->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);
    display_1->align_items(ui::LAYOUT_ALIGN_CENTER);

    m_input = std::make_unique<ui::input>(ui.theme<ui::input_theme>("dark"));
    m_input->width(200.0f);
    m_input->on_text_change = [this](std::string_view text) {
        if (text.empty())
            m_input_text->text(" ");
        else
            m_input_text->text(text);
    };
    display_1->add(m_input.get());

    m_input_text = std::make_unique<ui::label>(" ", ui.theme<ui::label_theme>("dark"));
    m_input_text->margin(200.0f, ui::LAYOUT_EDGE_LEFT);
    display_1->add(m_input_text.get());
}
} // namespace ash::sample