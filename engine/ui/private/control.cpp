#include "ui/control.hpp"
#include "layout/layout_node_yoga.hpp"
#include <queue>

namespace violet::ui
{
control::control(bool is_root)
    : m_layer(1),
      m_depth(1.0f),
      m_dirty(true),
      m_display(true),
      m_extent{},
      m_link_index(0),
      m_parent(nullptr)
{
    if (is_root)
        m_layout = std::make_unique<layout_root_yoga>();
    else
        m_layout = std::make_unique<layout_node_yoga>();

    m_event = std::make_unique<event_node>();
}

control::~control()
{
    VIOLET_ASSERT(m_children.empty());
    VIOLET_ASSERT(m_parent == nullptr);
}

void control::sync_extent()
{
    node_rect new_extent = m_layout->get_rect();
    if (new_extent.width != m_extent.width || new_extent.height != m_extent.height)
    {
        m_extent = new_extent;
        on_extent_change(m_extent.width, m_extent.height);

        if (event()->on_resize)
            event()->on_resize(m_extent.width, m_extent.height);
    }
    else if (new_extent.x != m_extent.x || new_extent.y != m_extent.y)
    {
        m_extent = new_extent;
    }
}

void control::add(control* child, std::size_t index)
{
    VIOLET_ASSERT(child && child->m_parent == nullptr);

    if (index == -1)
        index = m_children.size();

    m_layout->add_child(child->layout(), index);
    child->m_link_index = index;
    child->m_parent = this;

    auto iter = m_children.insert(m_children.begin() + index, child);
    while (++iter != m_children.end())
        ++(*iter)->m_link_index;

    child->update_depth(m_depth);

    control* node = this;
    while (node != nullptr)
    {
        node->on_add_child(child);
        node = node->m_parent;
    }
}

void control::remove(control* child)
{
    VIOLET_ASSERT(child->m_parent == this);

    auto iter = m_children.begin() + child->m_link_index;
    iter = m_children.erase(iter);

    for (; iter != m_children.end(); ++iter)
        --(*iter)->m_link_index;

    m_layout->remove_child(child->layout());
    child->m_depth = 0.0f;
    child->m_link_index = -1;
    child->m_parent = nullptr;

    control* node = this;
    while (node != nullptr)
    {
        node->on_remove_child(child);
        node = node->m_parent;
    }
}

void control::remove_from_parent()
{
    m_parent->remove(this);
}

void control::show()
{
    if (m_display)
        return;

    if (event()->on_show)
        event()->on_show();

    m_layout->set_display(true);
    m_display = true;
}

void control::hide()
{
    if (!m_display)
        return;

    if (event()->on_hide)
        event()->on_hide();

    m_layout->set_display(false);
    m_display = false;
}

void control::layer(int layer) noexcept
{
    m_layer = layer;

    if (m_parent != nullptr)
        update_depth(m_parent->depth());
}

void control::update_depth(float parent_depth) noexcept
{
    m_depth = parent_depth - 0.01f * m_layer;

    std::queue<control*> bfs;
    bfs.push(this);

    while (!bfs.empty())
    {
        control* node = bfs.front();
        bfs.pop();

        for (control* child : node->m_children)
        {
            child->m_depth = node->m_depth - 0.01f * child->m_layer;
            bfs.push(child);
        }
    }
}
} // namespace violet::ui