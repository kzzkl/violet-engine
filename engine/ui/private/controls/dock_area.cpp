#include "ui/controls/dock_area.hpp"
#include "ui/controls/dock_window.hpp"
#include "ui/controls/panel.hpp"
#include <queue>
#include <stdexcept>

namespace violet::ui
{
dock_area::dock_area(int control_width, int control_height, const dock_area_theme& theme)
    : m_area_width(control_width),
      m_area_height(control_height),
      m_docking_control(nullptr),
      m_resize_control(nullptr)
{
    layout()->set_width(control_width);
    layout()->set_height(control_height);

    m_hover_panel = std::make_unique<panel>(theme.hover_color);
    m_hover_panel->layout()->set_position_type(LAYOUT_POSITION_TYPE_ABSOLUTE);
    m_hover_panel->hide();
    m_hover_panel->layer(90);
    add(m_hover_panel.get());
}

void dock_area::dock(dock_node* root)
{
    add(root);
    root->dock_width(100.0f);
    root->dock_height(100.0f);
    root->m_dock_area = this;
}

void dock_area::dock(dock_node* source, dock_node* target, layout_edge edge)
{
    if (edge == LAYOUT_EDGE_ALL)
        return;

    undock(source);

    auto dock_direction = (edge == LAYOUT_EDGE_LEFT || edge == LAYOUT_EDGE_RIGHT)
                              ? LAYOUT_FLEX_DIRECTION_ROW
                              : LAYOUT_FLEX_DIRECTION_COLUMN;
    if (target->m_dock_parent == nullptr ||
        dock_direction != target->m_dock_parent->layout()->get_flex_direction())
    {
        move_down(target);
        target->m_dock_parent->layout()->set_flex_direction(dock_direction);
    }

    std::size_t link_index = 0;
    switch (edge)
    {
    case LAYOUT_EDGE_LEFT:
        source->dock_width(target->m_width / 3);
        source->dock_height(target->m_height);
        target->dock_width(target->m_width - source->m_width);
        link_index = target->link_index();
        break;
    case LAYOUT_EDGE_TOP:
        source->dock_width(target->m_width);
        source->dock_height(target->m_height / 3);
        target->dock_height(target->m_height - source->m_height);
        link_index = target->link_index();
        break;
    case LAYOUT_EDGE_RIGHT:
        source->dock_width(target->m_width / 3);
        source->dock_height(target->m_height);
        target->dock_width(target->m_width - source->m_width);
        link_index = target->link_index() + 1;
        break;
    case LAYOUT_EDGE_BOTTOM:
        source->dock_width(target->m_width);
        source->dock_height(target->m_height / 3);
        target->dock_height(target->m_height - source->m_height);
        link_index = target->link_index() + 1;
        break;
    default:
        break;
    }

    target->m_dock_parent->add(source, link_index);
    source->m_dock_parent = target->m_dock_parent;
}

void dock_area::undock(dock_node* control)
{
    std::size_t index = control->link_index();
    if (control->parent() != nullptr)
        control->remove_from_parent();

    if (control->m_dock_parent == nullptr)
        return;

    auto& brother_nodes = control->m_dock_parent->children();
    if (brother_nodes.size() == 1)
    {
        auto brother_node = static_cast<dock_node*>(brother_nodes[0]);
        move_up(brother_node);
    }
    else if (!brother_nodes.empty())
    {
        index = index == 0 ? 0 : index - 1;
        auto brother_node = static_cast<dock_node*>(brother_nodes[index]);

        if (control->m_dock_parent->layout()->get_flex_direction() == LAYOUT_FLEX_DIRECTION_ROW)
            brother_node->dock_width(brother_node->m_width + control->m_width);
        else
            brother_node->dock_height(brother_node->m_height + control->m_height);
    }
    else
    {
        control->m_dock_parent->remove_from_parent();
    }

    control->m_dock_parent = nullptr;
}

void dock_area::dock_begin(dock_node* control, int x, int y)
{
    m_docking_control = control;
}

void dock_area::dock_move(int x, int y)
{
    auto [node, edge] = find_mouse_over_control(x, y);
    if (node == m_docking_control || node == nullptr || edge == LAYOUT_EDGE_ALL)
    {
        m_hover_panel->hide();
    }
    else if (m_hover_node != node || m_hover_edge != edge)
    {
        auto& dock_extent = node->extent();
        auto& area_extent = extent();

        switch (edge)
        {
        case LAYOUT_EDGE_LEFT:
            m_hover_panel->layout()->set_width(dock_extent.width / 3);
            m_hover_panel->layout()->set_height(dock_extent.height);
            m_hover_panel->layout()->set_position(dock_extent.x - area_extent.x, LAYOUT_EDGE_LEFT);
            m_hover_panel->layout()->set_position(dock_extent.y - area_extent.y, LAYOUT_EDGE_TOP);
            break;
        case LAYOUT_EDGE_TOP:
            m_hover_panel->layout()->set_width(dock_extent.width);
            m_hover_panel->layout()->set_height(dock_extent.height / 3);
            m_hover_panel->layout()->set_position(dock_extent.x - area_extent.x, LAYOUT_EDGE_LEFT);
            m_hover_panel->layout()->set_position(dock_extent.y - area_extent.y, LAYOUT_EDGE_TOP);
            break;
        case LAYOUT_EDGE_RIGHT:
            m_hover_panel->layout()->set_width(dock_extent.width / 3);
            m_hover_panel->layout()->set_height(dock_extent.height);
            m_hover_panel->layout()->set_position(
                dock_extent.x + dock_extent.width / 3 * 2 - area_extent.x,
                LAYOUT_EDGE_LEFT);
            m_hover_panel->layout()->set_position(dock_extent.y - area_extent.y, LAYOUT_EDGE_TOP);
            break;
        case LAYOUT_EDGE_BOTTOM:
            m_hover_panel->layout()->set_width(dock_extent.width);
            m_hover_panel->layout()->set_height(dock_extent.height / 3);
            m_hover_panel->layout()->set_position(dock_extent.x - area_extent.x, LAYOUT_EDGE_LEFT);
            m_hover_panel->layout()->set_position(
                dock_extent.y + dock_extent.height / 3 * 2 - area_extent.y,
                LAYOUT_EDGE_TOP);
            break;
        default:
            break;
        }
        m_hover_panel->show();
    }

    m_hover_node = node;
    m_hover_edge = edge;
}

void dock_area::dock_end(int x, int y)
{
    auto [node, edge] = find_mouse_over_control(x, y);
    if (node == m_docking_control)
        return;

    if (node != nullptr)
    {
        dock(m_docking_control, node, edge);
        m_hover_panel->hide();
    }
}

void dock_area::resize(dock_node* control, layout_edge edge, int offset)
{
    if (offset == 0)
        return;

    auto resize_direction = (edge == LAYOUT_EDGE_LEFT || edge == LAYOUT_EDGE_RIGHT)
                                ? LAYOUT_FLEX_DIRECTION_ROW
                                : LAYOUT_FLEX_DIRECTION_COLUMN;

    m_resize_control = find_resize_control(control, edge);

    auto& brother_nodes = m_resize_control->m_dock_parent->children();
    if (edge == LAYOUT_EDGE_RIGHT)
    {
        m_resize_control =
            static_cast<dock_node*>(brother_nodes[m_resize_control->link_index() + 1]);
        edge = LAYOUT_EDGE_LEFT;
    }
    else if (edge == LAYOUT_EDGE_BOTTOM)
    {
        m_resize_control =
            static_cast<dock_node*>(brother_nodes[m_resize_control->link_index() + 1]);
        edge = LAYOUT_EDGE_TOP;
    }

    int index = m_resize_control->link_index();
    if (edge == LAYOUT_EDGE_LEFT)
    {
        float offset_percent = offset / m_resize_control->m_dock_parent->extent().width * 100.0f;
        float min_size = 50.0f / m_resize_control->m_dock_parent->extent().width * 100.0f;

        float a = 0.0f;
        if (offset_percent < 0)
        {
            offset_percent = -offset_percent;
            for (int i = index - 1; i >= 0; --i)
            {
                auto brother_node = static_cast<dock_node*>(brother_nodes[i]);
                float remain = brother_node->m_width - min_size;
                if (remain >= offset_percent)
                {
                    brother_node->dock_width(brother_node->m_width - offset_percent);
                    a += offset_percent;
                    offset_percent = 0;
                    break;
                }
                else
                {
                    brother_node->dock_width(min_size);
                    offset_percent -= remain;
                    a += remain;
                }
            }
            m_resize_control->dock_width(m_resize_control->m_width + a);
        }
        else
        {
            for (int i = index; i < brother_nodes.size(); ++i)
            {
                auto brother_node = static_cast<dock_node*>(brother_nodes[i]);
                float remain = brother_node->m_width - min_size;
                if (remain >= offset_percent)
                {
                    brother_node->dock_width(brother_node->m_width - offset_percent);
                    a += offset_percent;
                    offset_percent = 0;
                    break;
                }
                else
                {
                    brother_node->dock_width(min_size);
                    offset_percent -= remain;
                    a += remain;
                }
            }
            auto prev_node = static_cast<dock_node*>(brother_nodes[index - 1]);
            prev_node->dock_width(prev_node->m_width + a);
        }
    }
    else
    {
        float offset_percent = offset / m_resize_control->m_dock_parent->extent().height * 100.0f;
        float min_size = 50.0f / m_resize_control->m_dock_parent->extent().height * 100.0f;

        float a = 0.0f;
        if (offset_percent < 0)
        {
            offset_percent = -offset_percent;
            for (int i = index - 1; i >= 0; --i)
            {
                auto brother_node = static_cast<dock_node*>(brother_nodes[i]);
                float remain = brother_node->m_height - min_size;
                if (remain >= offset_percent)
                {
                    brother_node->dock_height(brother_node->m_height - offset_percent);
                    a += offset_percent;
                    offset_percent = 0;
                    break;
                }
                else
                {
                    brother_node->dock_height(min_size);
                    offset_percent -= remain;
                    a += remain;
                }
            }
            m_resize_control->dock_height(m_resize_control->m_height + a);
        }
        else
        {
            for (int i = index; i < brother_nodes.size(); ++i)
            {
                auto brother_node = static_cast<dock_node*>(brother_nodes[i]);
                float remain = brother_node->m_height - min_size;
                if (remain >= offset_percent)
                {
                    brother_node->dock_height(brother_node->m_height - offset_percent);
                    a += offset_percent;
                    offset_percent = 0;
                    break;
                }
                else
                {
                    brother_node->dock_height(min_size);
                    offset_percent -= remain;
                    a += remain;
                }
            }
            auto prev_node = static_cast<dock_node*>(brother_nodes[index - 1]);
            prev_node->dock_height(prev_node->m_height + a);
        }
    }
}

void dock_area::resize_end()
{
    if (m_resize_control == nullptr)
        return;

    std::queue<dock_node*> nodes;
    for (auto brother : m_resize_control->parent()->children())
        nodes.push(static_cast<dock_node*>(brother));

    while (!nodes.empty())
    {
        dock_node* node = nodes.front();
        nodes.pop();

        dock_window* window = dynamic_cast<dock_window*>(node);
        if (window == nullptr)
        {
            for (auto child : node->children())
                nodes.push(static_cast<dock_node*>(child));
        }
        else if (window->on_window_resize)
        {
            auto& window_extent = window->extent();
            window->on_window_resize(window_extent.width, window_extent.height);
        }
    }

    m_resize_control = nullptr;
}

void dock_area::move_up(dock_node* control)
{
    auto& parent = control->m_dock_parent;

    VIOLET_ASSERT(parent->children().size() == 1);

    control->dock_width(parent->m_width);
    control->dock_height(parent->m_height);

    if (parent->m_dock_parent == nullptr)
    {
        control->remove_from_parent();
        add(control);
        parent->remove_from_parent();
        control->m_dock_parent = nullptr;
    }
    else
    {
        control->remove_from_parent();
        parent->m_dock_parent->add(control, parent->link_index());
        parent->remove_from_parent();
        parent = parent->m_dock_parent;
    }
}

static std::size_t dock_counter = 0;
void dock_area::move_down(dock_node* control)
{
    auto new_dock_parent = std::make_shared<dock_node>(this);
    new_dock_parent->dock_width(control->m_width);
    new_dock_parent->dock_height(control->m_height);
    control->dock_width(100.0f);
    control->dock_height(100.0f);

    auto& parent = control->m_dock_parent;
    if (parent == nullptr)
    {
        control->parent()->add(new_dock_parent.get(), control->link_index());
    }
    else
    {
        parent->add(new_dock_parent.get(), control->link_index());
        new_dock_parent->m_dock_parent = parent;
    }

    control->remove_from_parent();
    new_dock_parent->add(control);
    parent = new_dock_parent;
}

std::pair<dock_node*, layout_edge> dock_area::find_mouse_over_control(int x, int y)
{
    if (children().empty())
        return {nullptr, LAYOUT_EDGE_ALL};

    std::queue<control*> bfs;
    for (auto child : children())
        bfs.push(child);

    std::pair<dock_node*, layout_edge> result = {nullptr, LAYOUT_EDGE_ALL};
    while (!bfs.empty())
    {
        dock_node* node = dynamic_cast<dock_node*>(bfs.front());
        bfs.pop();
        if (node == nullptr)
            continue;

        auto& e = node->extent();
        if (e.x < x && e.x + e.width > x && e.y < y && e.y + e.height > y)
        {
            result.first = node;
            for (auto child : node->children())
                bfs.push(child);
        }
    }

    if (result.first != nullptr)
    {
        auto& e = result.first->extent();
        if (x - e.x < e.width / 3)
            result.second = LAYOUT_EDGE_LEFT;
        else if (e.x + e.width - x < e.width / 3)
            result.second = LAYOUT_EDGE_RIGHT;
        else if (y - e.y < e.height / 3)
            result.second = LAYOUT_EDGE_TOP;
        else if (e.y + e.height - y < e.height / 3)
            result.second = LAYOUT_EDGE_BOTTOM;
        else
            result.second = LAYOUT_EDGE_ALL;
    }

    return result;
}

dock_node* dock_area::find_resize_control(dock_node* control, layout_edge edge)
{
    auto resize_direction = (edge == LAYOUT_EDGE_LEFT || edge == LAYOUT_EDGE_RIGHT)
                                ? LAYOUT_FLEX_DIRECTION_ROW
                                : LAYOUT_FLEX_DIRECTION_COLUMN;

    dock_node* resize_node = control;
    while (true)
    {
        if (resize_node->m_dock_parent->layout()->get_flex_direction() != resize_direction)
            resize_node = resize_node->m_dock_parent.get();

        switch (edge)
        {
        case LAYOUT_EDGE_LEFT:
        case LAYOUT_EDGE_TOP:
            if (resize_node->link_index() == 0)
                resize_node = resize_node->m_dock_parent.get();
            else
                return resize_node;
            break;
        case LAYOUT_EDGE_RIGHT:
        case LAYOUT_EDGE_BOTTOM:
            if (resize_node->link_index() == resize_node->m_dock_parent->children().size() - 1)
                resize_node = resize_node->m_dock_parent.get();
            else
                return resize_node;
            break;
        default:
            throw std::out_of_range("Invalid edge.");
        }
    }
}
} // namespace violet::ui