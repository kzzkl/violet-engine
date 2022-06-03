#include "gallery.hpp"
#include "style.hpp"
#include "ui/ui.hpp"

namespace ash::sample
{
void gallery::initialize()
{
    auto& ui = system<ui::ui>();

    ui.load_font(style::title_1.font, style::title_1.font_path, style::title_1.font_size);
    ui.load_font(style::content.font, style::content.font_path, style::content.font_size);

    ui.root()->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);

    m_left = std::make_unique<ui::panel>(style::background_color);
    m_left->resize(300.0f, 100.0f, false, false, false, true);
    m_left->link(ui.root());

    m_tree_page = std::make_unique<tree_page>();
    m_tree_page->resize(0.0f, 100.0f, true, false, false, true);
    m_tree_page->flex_grow(1.0f);
    m_tree_page->link(ui.root());
}
} // namespace ash::sample