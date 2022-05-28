#include "ui/controls/tree.hpp"
#include "window/window.hpp"

namespace ash::ui
{
tree_node::tree_node(
    std::string_view text,
    const font& font,
    std::uint32_t color,
    std::uint32_t background)
    : panel(background),
      m_label(std::make_unique<label>(text, font, color)),
      m_container(std::make_unique<container>())
{
    flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);
    resize(0.0f, 0.0f, true, true);
    padding(LAYOUT_EDGE_LEFT, 10.0f);

    m_label->link(this);
    m_label->resize(100.0f, 40.0f, false, false, true, false);

    m_label->on_mouse_click = [this](window::mouse_key key) {
        if (m_container->display())
            m_container->hide();
        else
            m_container->show();

        return false;
    };

    m_container->link(this);
    m_container->resize(0.0f, 0.0f, true, true);
    m_container->hide();
    m_container->padding(LAYOUT_EDGE_LEFT, 30);
    m_container->flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);
}

void tree_node::tick()
{
}

tree::tree()
{
    flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);
}
} // namespace ash::ui