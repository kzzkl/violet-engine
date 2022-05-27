#include "ui/element.hpp"

namespace ash::ui
{
element::element(bool is_root)
    : element_layout(is_root),
      m_layer(0),
      m_dirty(true),
      m_parent(nullptr)
{
}

void element::render(renderer& renderer)
{
    for (element* child : m_children)
        child->render(renderer);
}

void element::link(element* parent)
{
    if (m_parent != nullptr)
    {
        for (auto iter = m_parent->m_children.begin(); iter != m_parent->m_children.end(); ++iter)
        {
            if (*iter == this)
            {
                m_children.erase(iter);
                m_parent->on_remove_child(this);
                break;
            }
        }
    }

    if (parent != nullptr)
    {
        parent->m_children.push_back(this);
        parent->on_add_child(this);

        layout_parent(parent);
        m_layer = parent->m_layer + 1;
    }
    else
    {
        m_layer = 0;
    }

    m_parent = parent;
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
} // namespace ash::ui