#include "ui/element.hpp"
#include "log.hpp"

namespace ash::ui
{
element::element(bool is_root)
    : element_layout(is_root),
      mouse_over(false),
      m_layer(1),
      m_depth(1.0f),
      m_dirty(true),
      m_display(true),
      m_extent{},
      m_link_index(0),
      m_parent(nullptr)
{
}

element::~element()
{
    ASH_ASSERT(m_children.empty());
    ASH_ASSERT(m_parent == nullptr);

    // if (m_parent != nullptr)
    //     unlink();
}

void element::render(renderer& renderer)
{
    for (element* child : m_children)
    {
        if (child->m_display)
            child->render(renderer);
    }
}

void element::sync_extent()
{
    element_extent new_extent = layout_extent();
    if (new_extent != m_extent)
    {
        m_extent = new_extent;
        on_extent_change();
    }
}

void element::link(element* parent)
{
    link(parent, parent->children().size());
}

void element::link(element* parent, std::size_t index)
{
    ASH_ASSERT(parent && m_parent == nullptr);

    layout_link(parent, index);
    m_link_index = index;

    auto iter = parent->m_children.insert(parent->m_children.begin() + index, this);
    while (++iter != parent->m_children.end())
        ++(*iter)->m_link_index;

    parent->on_add_child(this);
    update_depth(parent->m_depth);

    m_parent = parent;
}

void element::unlink()
{
    if (m_parent == nullptr)
        return;

    layout_unlink();

    auto iter = m_parent->m_children.begin();
    for (; iter != m_parent->m_children.end(); ++iter)
    {
        if (*iter == this)
        {
            iter = m_parent->m_children.erase(iter);
            m_parent->on_remove_child(this);
            break;
        }
    }

    for (; iter != m_parent->m_children.end(); ++iter)
    {
        --(*iter)->m_link_index;
    }

    m_depth = 0.0f;
    m_parent = nullptr;
}

void element::show()
{
    if (m_display)
        return;

    if (on_show)
        on_show();

    layout_display(true);
    m_display = true;
}

void element::hide()
{
    if (!m_display)
        return;

    if (on_hide)
        on_hide();

    layout_display(false);
    m_display = false;
}

void element::layer(int layer) noexcept
{
    m_layer = layer;

    if (m_parent != nullptr)
        update_depth(m_parent->depth());
}

void element::on_add_child(element* child)
{
    if (m_parent != nullptr)
        m_parent->on_add_child(child);
}

void element::on_remove_child(element* child)
{
    if (m_parent != nullptr)
        m_parent->on_remove_child(child);
}

void element::update_depth(float parent_depth) noexcept
{
    m_depth = parent_depth - 0.01f * m_layer;

    for (element* child : m_children)
        child->update_depth(m_depth);
}
} // namespace ash::ui