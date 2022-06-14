#include "scroll_page.hpp"

namespace ash::sample
{
scroll_page::scroll_page()
{
    m_title = std::make_unique<text_title_1>("Scroll");
    m_title->link(this);

    m_description = std::make_unique<text_content>("You can use image control to display image.");
    m_description->link(this);

    for (std::size_t i = 0; i < 1; ++i)
    {
        auto display = std::make_unique<display_panel>();
        display->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);
        display->align_items(ui::LAYOUT_ALIGN_CENTER);
        m_display.push_back(std::move(display));
    }
    initialize_sample_scroll();
}

void scroll_page::initialize_sample_scroll()
{
    m_display[0]->name = "display";
    m_display[0]->link(this);

    m_scroll_view = std::make_unique<ui::scroll_view>();
    m_scroll_view->width(600.0f);
    m_scroll_view->height(400.0f);
    m_scroll_view->link(m_display[0].get());

    std::vector<std::uint32_t> color = {
        ui::COLOR_ANTIQUE_WHITE,
        ui::COLOR_AQUA,
        ui::COLOR_AQUA_MARINE,
        ui::COLOR_AZURE,
        ui::COLOR_BEIGE,
        ui::COLOR_BISQUE,
        ui::COLOR_BLACK,
        ui::COLOR_BLANCHEDALMOND,
        ui::COLOR_BLUE,
        ui::COLOR_BROWN,
        ui::COLOR_DARK_SALMON,
        ui::COLOR_DARK_SEA_GREEN};

    for (std::size_t i = 0; i < 8; ++i)
    {
        auto panel = std::make_unique<ui::panel>(color[i]);
        panel->name = "panel " + std::to_string(i);
        panel->width(200.0f);
        panel->height(200.0f);
        m_scroll_view->add(panel.get());
        m_panels.push_back(std::move(panel));
    }
}
} // namespace ash::sample