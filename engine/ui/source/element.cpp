#include "ui/element.hpp"

namespace ash::ui
{
element::element(bool is_root)
    : element_layout(is_root),
      m_depth(1.0f),
      m_dirty(true),
      m_display(true),
      m_parent(nullptr)
{
}

element::~element()
{
    for (auto child : m_children)
        child->unlink();

    if (m_parent != nullptr)
        unlink();
}

void element::render(renderer& renderer)
{
    for (element* child : m_children)
    {
        if (child->m_display)
            child->render(renderer);
    }
}

void element::link(element* parent)
{
    ASH_ASSERT(parent && m_parent == nullptr);

    layout_link(parent);

    parent->m_children.push_back(this);
    parent->on_add_child(this);
    update_depth(parent->m_depth);

    m_parent = parent;
}

void element::unlink()
{
    if (m_parent == nullptr)
        return;

    layout_unlink();

    for (auto iter = m_parent->m_children.begin(); iter != m_parent->m_children.end(); ++iter)
    {
        if (*iter == this)
        {
            m_parent->m_children.erase(iter);
            m_parent->on_remove_child(this);
            break;
        }
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

void element::update_depth(float parent_depth)
{
    m_depth = parent_depth - 0.01f;

    for (element* child : m_children)
        child->update_depth(m_depth);
}
} // namespace ash::ui