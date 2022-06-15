#include "page.hpp"
#include "ui/ui.hpp"

namespace ash::sample
{
page::page(std::string_view title)
{
    auto& ui = system<ui::ui>();

    padding(40.0f, ui::LAYOUT_EDGE_ALL);
    flex_direction(ui::LAYOUT_FLEX_DIRECTION_COLUMN);

    m_title = std::make_unique<ui::label>(title, ui.theme<ui::label_theme>("gallery title 1"));
    m_title->margin(20, ui::LAYOUT_EDGE_TOP);
    m_title->margin(10, ui::LAYOUT_EDGE_BOTTOM);
    m_title->link(this);
}

void page::add_subtitle(std::string_view subtitle)
{
    auto& ui = system<ui::ui>();

    auto subtitle_label =
        std::make_unique<ui::label>(subtitle, ui.theme<ui::label_theme>("gallery title 2"));
    subtitle_label->margin(10, ui::LAYOUT_EDGE_TOP);
    subtitle_label->margin(5, ui::LAYOUT_EDGE_BOTTOM);
    subtitle_label->link(this);
    m_subtitles.push_back(std::move(subtitle_label));
}

void page::add_description(std::string_view description)
{
    auto& ui = system<ui::ui>();

    auto description_label =
        std::make_unique<ui::label>(description, ui.theme<ui::label_theme>("dark"));
    description_label->margin(5, ui::LAYOUT_EDGE_TOP);
    description_label->margin(5, ui::LAYOUT_EDGE_BOTTOM);
    description_label->link(this);
    m_descriptions.push_back(std::move(description_label));
}

ui::panel* page::add_display_panel()
{
    auto& ui = system<ui::ui>();

    auto display_panel = std::make_unique<ui::panel>(0xFF44372E);
    display_panel->padding(20.0f, ui::LAYOUT_EDGE_ALL);
    display_panel->margin(20.0f, ui::LAYOUT_EDGE_TOP);
    display_panel->margin(20.0f, ui::LAYOUT_EDGE_BOTTOM);
    display_panel->link(this);
    m_display_panels.push_back(std::move(display_panel));

    return m_display_panels.back().get();
}
} // namespace ash::sample