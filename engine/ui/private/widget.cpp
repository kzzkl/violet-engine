#include "ui/widget.hpp"
#include "layout/layout_node_yoga.hpp"

namespace violet
{
widget::widget() : m_visible(true), m_index(0), m_layer(0)
{
    m_layout = std::make_unique<layout_node_yoga>();
}

widget::~widget()
{
}

void widget::add(std::shared_ptr<widget> child, std::size_t index)
{
    if (index == -1)
        index = m_children.size();

    m_layout->add_child(child->get_layout(), index);
    child->m_index = index;

    auto iter = m_children.insert(m_children.begin() + index, child);
    while (++iter != m_children.end())
        ++(*iter)->m_index;

    child->m_layer = m_layer + 1;
}

void widget::remove(std::shared_ptr<widget> child)
{
    auto iter = m_children.begin() + child->m_index;
    iter = m_children.erase(iter);

    for (; iter != m_children.end(); ++iter)
        --(*iter)->m_index;

    m_layout->remove_child(child->get_layout());
    child->m_index = -1;
    child->m_layer = 0;
}

widget_extent widget::get_extent() const
{
    return widget_extent{
        .x = m_layout->get_x(),
        .y = m_layout->get_y(),
        .width = m_layout->get_width(),
        .height = m_layout->get_height()};
}

void widget::paint(ui_draw_list* draw_list)
{
    on_paint(draw_list);
}
} // namespace violet