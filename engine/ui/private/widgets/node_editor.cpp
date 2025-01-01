#include "ui/widgets/node_editor.hpp"
#include "common/log.hpp"

namespace violet
{
node_editor::pin::pin()
{
    widget_layout* layout = get_layout();
    layout->set_width(20.0f);
    layout->set_height(20.0f);
}

void node_editor::pin::on_paint(ui_painter* painter)
{
    widget_extent extent = get_extent();
    painter->set_position(extent.x, extent.y);
    painter->set_color(ui_color::GREEN);
    painter->draw_rect(extent.width, extent.height);
}

node_editor::node::node()
{
    widget_layout* layout = get_layout();
    layout->set_position_type(LAYOUT_POSITION_TYPE_ABSOLUTE);
    layout->set_flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);
    layout->set_padding(title_size, LAYOUT_EDGE_TOP);
    layout->set_padding(10.0f, LAYOUT_EDGE_LEFT);
    layout->set_padding(10.0f, LAYOUT_EDGE_RIGHT);
    layout->set_padding(10.0f, LAYOUT_EDGE_BOTTOM);
    layout->set_width(200.0f);
    layout->set_height(160.0f);

    m_title = "Title";

    add_pin();
}

void node_editor::node::add_pin()
{
    add<pin>();
}

void node_editor::node::on_paint(ui_painter* painter)
{
    widget_extent extent = get_extent();
    painter->set_position(extent.x, extent.y);
    painter->set_color(ui_color(130, 32, 32, 200));
    painter->draw_rect(extent.width, title_size);

    painter->set_position(extent.x, extent.y + title_size);
    painter->set_color(ui_color(10, 10, 10, 200));
    painter->draw_rect(extent.width, extent.height - title_size);

    painter->set_position(
        extent.x + 20,
        extent.y + (title_size - static_cast<float>(painter->get_text_height(m_title))) * 0.5f);
    painter->set_color(ui_color::WHITE);
    painter->draw_text(m_title);
}

bool node_editor::node::on_drag_start(mouse_key key, int x, int y)
{
    if (key == MOUSE_KEY_LEFT)
    {
        m_drag_offset_x = get_layout()->get_absolute_x() - x;
        m_drag_offset_y = get_layout()->get_absolute_y() - y;
    }
    return false;
}

bool node_editor::node::on_drag(mouse_key key, int x, int y)
{
    if (key == MOUSE_KEY_LEFT)
    {
        widget_layout* layout = get_layout();
        float absolute_x = layout->get_absolute_x();
        float absolute_y = layout->get_absolute_y();

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
            .min_x = static_cast<std::uint32_t>(extent.x),
            .min_y = static_cast<std::uint32_t>(extent.y),
            .max_x = static_cast<std::uint32_t>(extent.x + extent.width),
            .max_y = static_cast<std::uint32_t>(extent.y + extent.height)});

    painter->set_position(extent.x, extent.y);
    painter->set_color(ui_color(20, 20, 25));
    painter->draw_rect(extent.width, extent.height);

    constexpr int grid_size = 50;

    int container_x = m_drag_offset_x - m_drag_start_x;
    int container_y = m_drag_offset_y - m_drag_start_y;

    int offset_x = container_x % grid_size;
    offset_x = offset_x < 0 ? offset_x + grid_size : offset_x;
    int offset_y = container_y % grid_size;

    std::vector<float2> path(2);

    painter->set_color(ui_color(120, 120, 120, 40));
    for (float x = offset_x; x < extent.width; x += grid_size)
    {
        path[0] = {x, 0};
        path[1] = {x, extent.height};
        painter->draw_path(path, 2.0f);
    }

    for (float y = offset_y; y < extent.height; y += grid_size)
    {
        path[0] = {0, y};
        path[1] = {extent.width, y};
        painter->draw_path(path, 2.0f);
    }
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
    else if (key == MOUSE_KEY_LEFT)
    {
    }

    return false;
}

bool node_editor::on_drag_start(mouse_key key, int x, int y)
{
    if (key == MOUSE_KEY_RIGHT)
    {
        m_drag_start_x = m_drag_offset_x = x;
        m_drag_start_y = m_drag_offset_y = y;
    }
    return false;
}

bool node_editor::on_drag(mouse_key key, int x, int y)
{
    if (key == MOUSE_KEY_RIGHT)
    {
        int offset_x = x - m_drag_offset_x;
        int offset_y = y - m_drag_offset_y;

        m_drag_offset_x = x;
        m_drag_offset_y = y;

        for (auto& child : get_children())
        {
            widget_layout* layout = child->get_layout();
            float absolute_x = layout->get_x();
            float absolute_y = layout->get_y();
            layout->set_position(absolute_x + offset_x, LAYOUT_EDGE_LEFT);
            layout->set_position(absolute_y + offset_y, LAYOUT_EDGE_TOP);
        }
    }
    return false;
}
} // namespace violet