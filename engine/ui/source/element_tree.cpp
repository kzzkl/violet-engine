#include "ui/element_tree.hpp"
#include "core/context.hpp"
#include "ui/ui_event.hpp"
#include "window/window.hpp"

namespace ash::ui
{
element_tree::element_tree()
    : m_hot_node(nullptr),
      m_focused_node(nullptr),
      m_window_width(0.0f),
      m_window_height(0.0f),
      m_tree_dirty(true)
{
    flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
}

void element_tree::tick()
{
    update_input();

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
        update_layout();

    if (layout_dirty || control_dirty)
        m_tree_dirty = true;
    else
        m_tree_dirty = false;
}

void element_tree::resize_window(float window_width, float window_height)
{
    resize(window_width, window_height);

    m_window_width = window_width;
    m_window_height = window_height;
}

void element_tree::update_input()
{
    auto& mouse = system<window::window>().mouse();
    if (mouse.mode() == window::MOUSE_MODE_RELATIVE)
        return;

    int mouse_x = mouse.x();
    int mouse_y = mouse.y();

    element* hot_node = nullptr;
    bfs(this, [&](element* node) {
        if (!node->display())
            return false;

        auto extent = node->extent();
        if (static_cast<int>(extent.x) < mouse_x &&
            static_cast<int>(extent.x + extent.width) > mouse_x &&
            static_cast<int>(extent.y) < mouse_y &&
            static_cast<int>(extent.y + extent.height) > mouse_y)
        {
            hot_node = node;
            return true;
        }
        else
        {
            return false;
        }
    });

    if (m_hot_node != hot_node)
    {
        if (m_hot_node != nullptr && m_hot_node->on_mouse_exit)
            m_hot_node->on_mouse_exit();

        if (hot_node != nullptr && hot_node->on_mouse_enter)
            hot_node->on_mouse_enter();

        m_hot_node = hot_node;
    }
    else if (hot_node != nullptr && hot_node->on_hover)
    {
        hot_node->on_hover();
    }

    if (hot_node == nullptr)
        return;

    // Bubble click event.
    bool key_down = false;
    if (mouse.key(window::MOUSE_KEY_LEFT).press())
    {
        bubble_click_event(hot_node, window::MOUSE_KEY_LEFT);
        key_down = true;
    }
    if (mouse.key(window::MOUSE_KEY_MIDDLE).press())
    {
        bubble_click_event(hot_node, window::MOUSE_KEY_MIDDLE);
        key_down = true;
    }
    if (mouse.key(window::MOUSE_KEY_RIGHT).press())
    {
        bubble_click_event(hot_node, window::MOUSE_KEY_RIGHT);
        key_down = true;
    }

    if (key_down && m_focused_node != hot_node)
    {
        if (m_focused_node != nullptr && m_focused_node->on_blur)
            m_focused_node->on_blur();

        if (hot_node->on_focus)
            hot_node->on_focus();

        m_focused_node = hot_node;
    }
}

void element_tree::update_layout()
{
    auto& event = system<core::event>();

    log::debug("calculate ui.");
    calculate(m_window_width, m_window_height);

    bfs(this, [&, this](element* node) -> bool {
        if (!node->display())
            return false;

        if (node->parent() != nullptr)
        {
            // The node coordinates stored in yoga are the relative coordinates of the parent node,
            // which are converted to absolute coordinates here.
            element_extent extent = node->parent()->extent();
            node->calculate_absolute_position(extent.x, extent.y);

            extent = node->extent();
            node->on_extent_change(extent);
        }
        return true;
    });

    event.publish<event_calculate_layout>();
}

void element_tree::bubble_click_event(element* node, window::mouse_key key)
{
    while (node != nullptr)
    {
        if (node->on_mouse_click && !node->on_mouse_click(key))
            return;

        node = node->parent();
    }
}

void element_tree::on_remove_child(element* child)
{
    if (m_hot_node == child)
        m_hot_node = nullptr;

    if (m_focused_node == child)
        m_focused_node = nullptr;
}
} // namespace ash::ui