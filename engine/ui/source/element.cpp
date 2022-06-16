#include "ui/element.hpp"

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
        on_extent_change(m_extent);

        if (on_resize)
            on_resize(m_extent.width, m_extent.height);
    }
}

void element::add(element* child, std::size_t index)
{
    on_add_child(child, index);
}

void element::remove(element* child)
{
    on_remove_child(child);
}

void element::remove_from_parent()
{
    m_parent->remove(this);
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

void element::on_depth_change(float depth)
{
    for (auto& vertex : m_mesh.vertex_position)
        vertex[2] = depth;
}

void element::on_add_child(element* child, std::size_t index)
{
    ASH_ASSERT(child && child->m_parent == nullptr);

    if (index == -1)
        index = m_children.size();

    layout_add_child(child, index);
    child->m_link_index = index;
    child->m_parent = this;

    auto iter = m_children.insert(m_children.begin() + index, child);
    while (++iter != m_children.end())
        ++(*iter)->m_link_index;

    child->update_depth(m_depth);
}

void element::on_remove_child(element* child)
{
    ASH_ASSERT(child->m_parent == this);

    auto iter = m_children.begin() + child->m_link_index;
    iter = m_children.erase(iter);

    for (; iter != m_children.end(); ++iter)
        --(*iter)->m_link_index;

    layout_remove_child(child);
    child->m_depth = 0.0f;
    child->m_link_index = -1;
    child->m_parent = nullptr;
}

void element::update_depth(float parent_depth) noexcept
{
    m_depth = parent_depth - 0.01f * m_layer;
    on_depth_change(m_depth);

    for (element* child : m_children)
        child->update_depth(m_depth);
}
} // namespace ash::ui