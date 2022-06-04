#include "gallery_control.hpp"
#include "style.hpp"
#include "ui/ui.hpp"

namespace ash::sample
{
text_title_1::text_title_1(std::string_view content, std::uint32_t color)
    : label(content, system<ui::ui>().font("title 1"), color)
{
    margin(style::title_1.margin_top, ui::LAYOUT_EDGE_TOP);
    margin(style::title_1.margin_bottom, ui::LAYOUT_EDGE_BOTTOM);
}

text_title_2::text_title_2(std::string_view content, std::uint32_t color)
    : label(content, system<ui::ui>().font("title 2"), color)
{
    margin(style::title_2.margin_top, ui::LAYOUT_EDGE_TOP);
    margin(style::title_2.margin_bottom, ui::LAYOUT_EDGE_BOTTOM);
}

text_content::text_content(std::string_view content, std::uint32_t color)
    : label(content, system<ui::ui>().font("content"), color)
{
    margin(style::content.margin_top, ui::LAYOUT_EDGE_TOP);
    margin(style::content.margin_bottom, ui::LAYOUT_EDGE_BOTTOM);
}

page::page()
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