#include "ui/widgets/node_editor.hpp"
#include "common/log.hpp"

namespace violet
{
node_editor::pin::pin()
{
}

node_editor::node::node()
{
    widget_layout* layout = get_layout();
    layout->set_position_type(LAYOUT_POSITION_TYPE_ABSOLUTE);
    layout->set_flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);
    layout->set_padding(30.0f, LAYOUT_EDGE_TOP);
    layout->set_padding(5.0f, LAYOUT_EDGE_LEFT);
    layout->set_padding(5.0f, LAYOUT_EDGE_RIGHT);
    layout->set_padding(5.0f, LAYOUT_EDGE_BOTTOM);
    layout->set_width(200.0f);
    layout->set_height(160.0f);

    m_title = "Title";
}

void node_editor::node::on_paint(ui_painter* painter)
{
    widget_extent extent = get_extent();
    painter->set_position(extent.x, extent.y);
    painter->set_color(COLOR_BROWN);
    painter->draw_rect(extent.width, extent.height);

    painter->set_position(extent.x + 5, extent.y + (30 - painter->get_text_height(m_title)) / 2);
    painter->set_color(COLOR_WHITE);
    painter->draw_text(m_title);
}

bool node_editor::node::on_drag_start(mouse_key key, int x, int y)
{
    if (key == MOUSE_KEY_LEFT)
    {
        m_drag_offset_x = get_layout()->get_x() - x;
        m_drag_offset_y = get_layout()->get_y() - y;
    }
    return false;
}

bool node_editor::node::on_drag(mouse_key key, int x, int y)
{
    if (key == MOUSE_KEY_LEFT)
    {
        widget_layout* layout = get_layout();
        std::uint32_t absolute_x = layout->get_absolute_x();
        std::uint32_t absolute_y = layout->get_absolute_y();

        layout->set_position(m_drag_offset_x + x, LAYOUT_EDGE_LEFT);
        layout->set_position(m_drag_offset_y + y, LAYOUT_EDGE_TOP);
    }
    return false;
}

node_editor::node_editor()
{
    widget_layout* layout = get_layout();
    layout->set_flex_grow(1.0f);
}

node_editor::node* node_editor::add_node()
{
    return add<node>();
}

void node_editor::on_paint(ui_painter* painter)
{
    widget_extent extent = get_extent();
    painter->push_group(
        true,
        rhi_scissor_rect{
            .min_x = extent.x,
            .min_y = extent.y,
            .max_x = extent.x + extent.width,
            .max_y = extent.y + extent.height});
}

void node_editor::on_paint_end(ui_painter* painter)
{
    painter->pop_group();
}

bool node_editor::on_mouse_click(mouse_key key, int x, int y)
{
    if (key == MOUSE_KEY_RIGHT)
    {
        node* node = add_node();
        node->get_layout()->set_position(x, LAYOUT_EDGE_LEFT);
        node->get_layout()->set_position(y, LAYOUT_EDGE_TOP);
    }

    return false;
}
} // namespace violet