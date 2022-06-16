#include "scroll_page.hpp"
#include "ui/ui.hpp"

namespace ash::sample
{
scroll_page::scroll_page() : page("Scroll")
{
    add_description("You can use image control to display image.");

    initialize_sample_scroll();
}

void scroll_page::initialize_sample_scroll()
{
    auto& ui = system<ui::ui>();

    auto display_1 = add_display_panel();
    display_1->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);
    display_1->align_items(ui::LAYOUT_ALIGN_CENTER);

    m_scroll_view = std::make_unique<ui::scroll_view>(ui.theme<ui::scroll_view_theme>("dark"));
    m_scroll_view->width(600.0f);
    m_scroll_view->height(400.0f);
    display_1->add(m_scroll_view.get());

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
        m_scroll_view->add_item(panel.get());
        m_panels.push_back(std::move(panel));
    }
}
} // namespace ash::sample