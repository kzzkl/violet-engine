#include "gallery_control.hpp"
#include "style.hpp"
#include "ui/ui.hpp"

namespace ash::sample
{
text_title_1::text_title_1(std::string_view content, std::uint32_t color)
{
    auto& ui = system<ui::ui>();
    reset(content, ui.font("title 1"), color);
    margin(style::title_1.margin_top, ui::LAYOUT_EDGE_TOP);
    margin(style::title_1.margin_bottom, ui::LAYOUT_EDGE_BOTTOM);
}

text_content::text_content(std::string_view content, std::uint32_t color)
{
    auto& ui = system<ui::ui>();
    reset(content, ui.font("content"), color);
    margin(style::content.margin_top, ui::LAYOUT_EDGE_TOP);
    margin(style::content.margin_bottom, ui::LAYOUT_EDGE_BOTTOM);
}

page::page() : ui::panel(style::page_color)
{
    padding(40.0f, ui::LAYOUT_EDGE_ALL);
    flex_direction(ui::LAYOUT_FLEX_DIRECTION_COLUMN);
}

display_panel::display_panel() : ui::panel(style::display_color)
{
    padding(20.0f, ui::LAYOUT_EDGE_ALL);
    margin(20.0f, ui::LAYOUT_EDGE_TOP);
    margin(20.0f, ui::LAYOUT_EDGE_BOTTOM);
}
} // namespace ash::sample