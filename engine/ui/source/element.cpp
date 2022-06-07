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

class dock_intermediate_node : public dock_element
{
public:
    virtual bool dockable() const noexcept { return false; }
};

dock_element::dock_element()
{
}

void dock_element::dock(dock_element* target, layout_edge edge)
{
    undock();

    width_auto();
    height_auto();
    flex_grow(1.0f);

    auto dock_direction = (edge == LAYOUT_EDGE_LEFT || edge == LAYOUT_EDGE_RIGHT)
                              ? LAYOUT_FLEX_DIRECTION_ROW
                              : LAYOUT_FLEX_DIRECTION_COLUMN;

    element* target_parent = target->parent();
    if (!target->is_root() && dock_direction == target_parent->flex_direction())
    {
        if (edge == LAYOUT_EDGE_LEFT || edge == LAYOUT_EDGE_TOP)
            link(target_parent, target->link_index());
        else
            link(target_parent, target->link_index() + 1);
        m_dock_node = target->m_dock_node;
    }
    else
    {
        target->move_down();
        target->width_auto();
        target->height_auto();
        target->flex_grow(1.0f);
        target->m_dock_node->flex_direction(dock_direction);

        if (edge == LAYOUT_EDGE_LEFT || edge == LAYOUT_EDGE_TOP)
            link(target->m_dock_node.get(), 0);
        else
            link(target->m_dock_node.get());
        m_dock_node = target->m_dock_node;
    }
}

void dock_element::undock()
{
    if (is_root())
    {
        unlink();
    }
    else
    {
        auto parent_node = static_cast<dock_element*>(parent());

        unlink();
        m_dock_node = nullptr;

        if (parent_node->children().size() == 1)
        {
            dock_element* brother_node = static_cast<dock_element*>(parent_node->children()[0]);
            brother_node->move_up();
        }
    }
}

void dock_element::move_up()
{
    ASH_ASSERT(m_dock_node != nullptr);
    ASH_ASSERT(m_dock_node->children().size() == 1);

    auto parent_node = static_cast<dock_element*>(parent());
    if (parent_node->is_root())
        parent_node->copy_style(this);

    unlink();
    link(parent_node->parent(), parent_node->link_index());
    m_dock_node = parent_node->m_dock_node;
}

void dock_element::move_down()
{
    auto dock_node = std::make_shared<dock_intermediate_node>();

    dock_node->link(parent(), link_index());
    if (is_root())
        copy_style(dock_node.get());
    else
        dock_node->flex_grow(1.0f);
    dock_node->m_dock_node = m_dock_node;

    unlink();
    link(dock_node.get());

    m_dock_node = dock_node;
}
} // namespace ash::ui