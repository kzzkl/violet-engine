#include "control_tree.hpp"

namespace violet
{
namespace
{
/*
inline bool in_extent(int x, int y, const node_rect& extent)
{
    return extent.x <= x && extent.x + extent.width >= x && extent.y <= y &&
           extent.y + extent.height >= y;
}
*/
} // namespace

control_tree::control_tree()
    : /*m_hot_node(nullptr),
      m_focused_node(nullptr),
      m_drag_node(nullptr),*/
      m_tree_dirty(true)
{
    // layout()->set_flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
}

void control_tree::tick(float width, float height)
{/*
    update_input();

    bool layout_dirty = false;
    bool control_dirty = false;

    while (true)
    {
        bool layout_update_flag = false;

        bfs(this,
            [&](control* node) -> bool
            {
                if (!node->display())
                    return false;

                if (node->layout()->dirty())
                {
                    layout_dirty = true;
                    layout_update_flag = true;
                }

                if (node->control_dirty())
                {
                    control_dirty = true;
                    node->reset_control_dirty();
                }

                return true;
            });

        if (layout_update_flag)
            update_layout(width, height);
        else
            break;
    }

    if (layout_dirty || control_dirty)
        m_tree_dirty = true;
    else
        m_tree_dirty = false;*/
}

void control_tree::update_input()
{
    /*
    auto& mouse = system<window>().mouse();
    if (mouse.mode() == MOUSE_MODE_RELATIVE)
        return;
    int mouse_x = 0;
    int mouse_y = 0;

    for (control* node : m_mouse_over_nodes)
    {
        if (!in_extent(mouse_x, mouse_y, node->extent()))
        {
            node->event()->mouse_over = false;
            if (node->event()->on_mouse_out)
                node->event()->on_mouse_out();
        }
    }
    m_mouse_over_nodes.clear();

    control* hot_node = nullptr;
    float hot_node_depth = 1.0f;

    control* focused_node = nullptr;
    float focused_node_depth = 1.0f;

    control* drag_node = nullptr;
    float drag_node_depth = 1.0f;

    bfs(this,
        [&](control* node) -> bool
        {
            if (!node->display())
                return false;

            auto extent = node->extent();
            if (in_extent(mouse_x, mouse_y, node->extent()))
            {
                if (!node->event()->mouse_over)
                {
                    node->event()->mouse_over = true;
                    if (node->event()->on_mouse_over)
                        node->event()->on_mouse_over();
                }
                m_mouse_over_nodes.push_back(node);

                if (hot_node_depth > node->depth())
                {
                    hot_node = node;
                    hot_node_depth = node->depth();
                }

                if (focused_node_depth > node->depth() && node->event()->on_focus)
                {
                    focused_node = node;
                    focused_node_depth = node->depth();
                }

                if (drag_node_depth > node->depth() &&
                    (node->event()->on_mouse_drag_begin || node->event()->on_mouse_drag ||
                     node->event()->on_mouse_drag_end))
                {
                    drag_node = node;
                    drag_node_depth = node->depth();
                }

                return true;
            }
            else
            {
                return false;
            }
        });

    for (auto node : m_mouse_over_nodes)
    {
        if (node->event()->on_mouse_move)
            node->event()->on_mouse_move(mouse_x, mouse_y);
    }

    bubble_mouse_event(hot_node, focused_node, drag_node);
    */
}

void control_tree::update_layout(float width, float height)
{/*
    layout()->calculate(width, height);

    bfs(this,
        [&, this](control* node) -> bool
        {
            if (!node->display())
                return false;

            if (node->parent() != nullptr)
            {
                // The node coordinates stored in yoga are the relative coordinates of the parent
                // node, which are converted to absolute coordinates here.
                node_rect extent = node->parent()->extent();
                node->layout()->calculate_absolute_position(extent.x, extent.y);
            }
            node->sync_extent();

            return true;
        });*/
}

/* void control_tree::bubble_mouse_event(control* hot_node, control* focused_node, control* drag_node)
{
    
    auto& mouse = system<window>().mouse();
    int mouse_x = mouse.x();
    int mouse_y = mouse.y();

    static const std::vector<mouse_key> keys = {MOUSE_KEY_LEFT, MOUSE_KEY_RIGHT, MOUSE_KEY_MIDDLE};

    bool key_down = false;
    for (auto key : keys)
    {
        if (mouse.key(key).press())
        {
            control* node = hot_node;
            while (node != nullptr)
            {
                if (node->event()->on_mouse_press &&
                    !node->event()->on_mouse_press(key, mouse_x, mouse_y))
                    break;
                node = node->parent();
            }
            key_down = true;
        }

        if (mouse.key(key).release())
        {
            control* node = hot_node;
            while (node != nullptr)
            {
                if (node->event()->on_mouse_release &&
                    !node->event()->on_mouse_release(key, mouse_x, mouse_y))
                    break;
                node = node->parent();
            }
        }

        if (mouse.whell() != 0)
        {
            control* node = hot_node;
            while (node != nullptr)
            {
                if (node->event()->on_mouse_wheel && !node->event()->on_mouse_wheel(mouse.whell()))
                    break;
                node = node->parent();
            }
        }
    }

    if (mouse.key(MOUSE_KEY_LEFT).down())
    {
        if (m_drag_node == nullptr)
        {
            if (drag_node != nullptr)
            {
                m_drag_node = drag_node;
                if (m_drag_node->event()->on_mouse_drag_begin)
                    m_drag_node->event()->on_mouse_drag_begin(mouse_x, mouse_y);
            }
        }
        else
        {
            if (m_drag_node->event()->on_mouse_drag)
                m_drag_node->event()->on_mouse_drag(mouse_x, mouse_y);
        }
    }
    else
    {
        if (m_drag_node != nullptr)
        {
            if (m_drag_node->event()->on_mouse_drag_end)
                m_drag_node->event()->on_mouse_drag_end(mouse_x, mouse_y);
            m_drag_node = nullptr;
        }
    }

    if (m_hot_node != hot_node)
    {
        if (m_hot_node != nullptr && m_hot_node->event()->on_mouse_leave)
            m_hot_node->event()->on_mouse_leave();

        if (hot_node != nullptr && hot_node->event()->on_mouse_enter)
            hot_node->event()->on_mouse_enter();

        m_hot_node = hot_node;
    }

    if (focused_node != m_focused_node && key_down)
    {
        if (m_focused_node != nullptr && m_focused_node->event()->on_blur)
            m_focused_node->event()->on_blur();

        if (focused_node != nullptr && focused_node->event()->on_focus)
            focused_node->event()->on_focus();

        m_focused_node = focused_node;
    }

    if (m_focused_node != nullptr && m_focused_node->event()->on_input)
    {
        while (!m_input_chars.empty())
        {
            m_focused_node->event()->on_input(m_input_chars.front());
            m_input_chars.pop();
        }
    }
    else
    {
        while (!m_input_chars.empty())
            m_input_chars.pop();
    }
}
*/
/*
void control_tree::on_remove_child(control* child)
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
}*/
} // namespace violet