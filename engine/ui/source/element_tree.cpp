#include "ui/element_tree.hpp"
#include "core/context.hpp"
#include "ui/ui_event.hpp"
#include "window/window.hpp"

namespace ash::ui
{
namespace
{
inline bool in_extent(int x, int y, const element_extent& extent)
{
    return extent.x <= x && extent.x + extent.width >= x && extent.y <= y &&
           extent.y + extent.height >= y;
}
} // namespace

element_tree::element_tree()
    : m_hot_node(nullptr),
      m_focused_node(nullptr),
      m_drag_node(nullptr),
      m_dock_node(nullptr),
      m_tree_dirty(true)
{
    flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
}

void element_tree::tick(float width, float height)
{
    update_input();
    // update_docking();

    bool layout_dirty = false;
    bool control_dirty = false;

    bfs(this, [&](element* node) -> bool {
        if (!node->display())
            return false;

        if (node->layout_dirty())
            layout_dirty = true;

        if (node->control_dirty())
        {
            control_dirty = true;
            node->reset_control_dirty();
        }

        return true;
    });

    if (layout_dirty)
        update_layout(width, height);

    if (layout_dirty || control_dirty)
        m_tree_dirty = true;
    else
        m_tree_dirty = false;
}

void element_tree::update_input()
{
    auto& mouse = system<window::window>().mouse();
    if (mouse.mode() == window::MOUSE_MODE_RELATIVE)
        return;

    int mouse_x = mouse.x();
    int mouse_y = mouse.y();

    for (element* node : m_mouse_over_nodes)
    {
        if (!in_extent(mouse_x, mouse_y, node->extent()))
        {
            node->mouse_over = false;
            if (node->on_mouse_out)
                node->on_mouse_out();
        }
    }
    m_mouse_over_nodes.clear();

    bfs(this, [&](element* node) -> bool {
        if (!node->display())
            return false;

        auto extent = node->extent();
        if (in_extent(mouse_x, mouse_y, node->extent()))
        {
            if (!node->mouse_over)
            {
                node->mouse_over = true;
                if (node->on_mouse_over)
                    node->on_mouse_over();
            }
            m_mouse_over_nodes.push_back(node);
            return true;
        }
        else
        {
            return false;
        }
    });

    for (auto node : m_mouse_over_nodes)
    {
        if (node->on_mouse_move)
            node->on_mouse_move(mouse_x, mouse_y);
    }

    bubble_mouse_event(m_mouse_over_nodes.empty() ? nullptr : m_mouse_over_nodes.back());
}

/*void element_tree::update_docking()
{
    if (m_dock_node == nullptr && m_drag_node && m_drag_node->dockable())
    {
        m_dock_node = static_cast<dock_element*>(m_drag_node);
    }

    if (m_drag_node == nullptr && m_dock_node != nullptr)
    {
        dock_element* dock_target_node = nullptr;
        for (element* node : m_mouse_over_nodes)
        {
            if (node != m_dock_node && node->dockable())
                dock_target_node = static_cast<dock_element*>(node);
        }

        if (dock_target_node != nullptr)
        {
            auto& mouse = system<window::window>().mouse();
            int mouse_x = mouse.x();
            int mouse_y = mouse.y();

            auto& extent = dock_target_node->extent();

            layout_edge edge;
            float min_distance = 100000.0f;

            float distance = mouse_x - extent.x;
            if (distance < min_distance)
            {
                edge = LAYOUT_EDGE_LEFT;
                min_distance = distance;
            }
            distance = extent.x + extent.width - mouse_x;
            if (distance < min_distance)
            {
                edge = LAYOUT_EDGE_RIGHT;
                min_distance = distance;
            }
            distance = mouse_y - extent.y;
            if (distance < min_distance)
            {
                edge = LAYOUT_EDGE_TOP;
                min_distance = distance;
            }
            distance = extent.y + extent.height - mouse_y;
            if (distance < min_distance)
            {
                edge = LAYOUT_EDGE_BOTTOM;
                min_distance = distance;
            }

            m_dock_node->dock(dock_target_node, edge);
        }

        m_dock_node = nullptr;
    }
}*/

void element_tree::update_layout(float width, float height)
{
    auto& event = system<core::event>();

    log::debug("calculate ui.");
    calculate(width, height);

    bfs(this, [&, this](element* node) -> bool {
        if (!node->display())
            return false;

        if (node->parent() != nullptr)
        {
            // The node coordinates stored in yoga are the relative coordinates of the parent node,
            // which are converted to absolute coordinates here.
            element_extent extent = node->parent()->extent();
            node->calculate_absolute_position(extent.x, extent.y);
        }
        node->sync_extent();

        return true;
    });

    event.publish<event_calculate_layout>();
}

void element_tree::bubble_mouse_event(element* hot_node)
{
    auto& mouse = system<window::window>().mouse();
    int mouse_x = mouse.x();
    int mouse_y = mouse.y();

    static const std::vector<window::mouse_key> keys = {
        window::MOUSE_KEY_LEFT,
        window::MOUSE_KEY_RIGHT,
        window::MOUSE_KEY_MIDDLE};

    bool key_down = false;
    for (auto key : keys)
    {
        if (mouse.key(key).press())
        {
            element* node = hot_node;
            while (node != nullptr)
            {
                if (node->on_mouse_press && !node->on_mouse_press(key, mouse_x, mouse_y))
                    break;
                node = node->parent();
            }
            key_down = true;
        }

        if (mouse.key(key).release())
        {
            element* node = hot_node;
            while (node != nullptr)
            {
                if (node->on_mouse_release && !node->on_mouse_release(key, mouse_x, mouse_y))
                    break;
                node = node->parent();
            }
        }

        if (mouse.whell() != 0)
        {
            element* node = hot_node;
            while (node != nullptr)
            {
                if (node->on_mouse_wheel && !node->on_mouse_wheel(mouse.whell()))
                    break;
                node = node->parent();
            }
        }
    }

    if (mouse.key(window::MOUSE_KEY_LEFT).down())
    {
        if (m_drag_node == nullptr)
        {
            m_drag_node = hot_node;
            if (m_drag_node->on_mouse_drag_begin)
                m_drag_node->on_mouse_drag_begin(mouse_x, mouse_y);
        }
        else
        {
            if (m_drag_node->on_mouse_drag)
                m_drag_node->on_mouse_drag(mouse_x, mouse_y);
        }
    }
    else
    {
        if (m_drag_node != nullptr)
        {
            if (m_drag_node->on_mouse_drag_end)
                m_drag_node->on_mouse_drag_end(mouse_x, mouse_y);
            m_drag_node = nullptr;
        }
    }

    if (m_hot_node != hot_node)
    {
        if (m_hot_node != nullptr && m_hot_node->on_mouse_leave)
            m_hot_node->on_mouse_leave();

        if (hot_node != nullptr && hot_node->on_mouse_enter)
            hot_node->on_mouse_enter();

        m_hot_node = hot_node;
    }

    if (hot_node != nullptr && key_down && m_focused_node != hot_node)
    {
        if (m_focused_node != nullptr && m_focused_node->on_blur)
            m_focused_node->on_blur();

        if (hot_node->on_focus)
            hot_node->on_focus();

        m_focused_node = hot_node;
    }
}

void element_tree::on_remove_child(element* child)
{
    if (m_hot_node == child)
        m_hot_node = nullptr;

    if (m_focused_node == child)
        m_focused_node = nullptr;

    for (auto& node : m_mouse_over_nodes)
    {
        if (node == child)
        {
            node = m_mouse_over_nodes.back();
            m_mouse_over_nodes.pop_back();
            break;
        }
    }
}
} // namespace ash::ui