#pragma once

#include "element_extent.hpp"
#include <memory>

namespace ash::ui
{
enum layout_direction
{
    LAYOUT_DIRECTION_INHERIT,
    LAYOUT_DIRECTION_LTR,
    LAYOUT_DIRECTION_RTL
};

enum layout_flex_direction
{
    LAYOUT_FLEX_DIRECTION_COLUMN,
    LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE,
    LAYOUT_FLEX_DIRECTION_ROW,
    LAYOUT_FLEX_DIRECTION_ROW_REVERSE
};

enum layout_flex_wrap
{
    LAYOUT_FLEX_WRAP_NOWRAP,
    LAYOUT_FLEX_WRAP_WRAP,
    LAYOUT_FLEX_WRAP_WRAP_REVERSE
};

enum layout_justify
{
    LAYOUT_JUSTIFY_FLEX_START,
    LAYOUT_JUSTIFY_CENTER,
    LAYOUT_JUSTIFY_FLEX_END,
    LAYOUT_JUSTIFY_SPACE_BETWEEN,
    LAYOUT_JUSTIFY_SPACE_AROUND,
    LAYOUT_JUSTIFY_SPACE_EVENLY
};

enum layout_align
{
    LAYOUT_ALIGN_AUTO,
    LAYOUT_ALIGN_FLEX_START,
    LAYOUT_ALIGN_CENTER,
    LAYOUT_ALIGN_FLEX_END,
    LAYOUT_ALIGN_STRETCH,
    LAYOUT_ALIGN_BASELINE,
    LAYOUT_ALIGN_SPACE_BETWEEN,
    LAYOUT_ALIGN_SPACE_AROUND
};

enum layout_edge
{
    LAYOUT_EDGE_LEFT,
    LAYOUT_EDGE_TOP,
    LAYOUT_EDGE_RIGHT,
    LAYOUT_EDGE_BOTTOM,
    LAYOUT_EDGE_START,
    LAYOUT_EDGE_END,
    LAYOUT_EDGE_HORIZONTAL,
    LAYOUT_EDGE_VERTICAL,
    LAYOUT_EDGE_ALL
};

enum layout_position_type
{
    LAYOUT_POSITION_TYPE_STATIC,
    LAYOUT_POSITION_TYPE_RELATIVE,
    LAYOUT_POSITION_TYPE_ABSOLUTE
};

class layout_node_impl
{
public:
    virtual void direction(layout_direction direction) = 0;
    virtual void flex_direction(layout_flex_direction flex_direction) = 0;
    virtual void flex_basis(float basis) = 0;
    virtual void flex_grow(float grow) = 0;
    virtual void flex_shrink(float shrink) = 0;
    virtual void flex_wrap(layout_flex_wrap wrap) = 0;
    virtual void justify_content(layout_justify justify) = 0;
    virtual void align_items(layout_align align) = 0;
    virtual void align_self(layout_align align) = 0;
    virtual void align_content(layout_align align) = 0;
    virtual void padding(float padding, layout_edge edge) = 0;
    virtual void border(float border, layout_edge edge) = 0;
    virtual void margin(float margin, layout_edge edge) = 0;
    virtual void display(bool display) = 0;
    virtual void position_type(layout_position_type position_type) = 0;
    virtual void position(float position, layout_edge edge, bool percent) = 0;

    virtual layout_direction direction() const = 0;
    virtual layout_flex_direction flex_direction() const = 0;
    virtual float flex_basis() const = 0;
    virtual float flex_grow() const = 0;
    virtual float flex_shrink() const = 0;
    virtual layout_flex_wrap flex_wrap() const = 0;
    virtual layout_justify justify_content() const = 0;
    virtual layout_align align_items() const = 0;
    virtual layout_align align_self() const = 0;
    virtual layout_align align_content() const = 0;
    virtual float padding(layout_edge edge) const = 0;
    virtual float border(layout_edge edge) const = 0;
    virtual float margin(layout_edge edge) const = 0;

    virtual void add_child(layout_node_impl* child, std::size_t index) = 0;
    virtual void remove_child(layout_node_impl* child) = 0;

    virtual void calculate(float width, float height) = 0;
    virtual void calculate_absolute_position(float parent_x, float parent_y) = 0;

    virtual void width(float value) = 0;
    virtual void width_auto() = 0;
    virtual void width_percent(float value) = 0;
    virtual void width_min(float value) = 0;
    virtual void width_max(float value) = 0;

    virtual void height(float value) = 0;
    virtual void height_auto() = 0;
    virtual void height_percent(float value) = 0;
    virtual void height_min(float value) = 0;
    virtual void height_max(float value) = 0;

    virtual void copy_style(layout_node_impl* target) = 0;

    virtual element_extent extent() const = 0;
    virtual bool dirty() const = 0;
};

class element_layout
{
public:
    element_layout(bool is_root);

    void direction(layout_direction direction) { m_impl->direction(direction); }
    void flex_direction(layout_flex_direction flex_direction)
    {
        m_impl->flex_direction(flex_direction);
    }
    void flex_basis(float basis) { m_impl->flex_basis(basis); }
    void flex_grow(float grow) { m_impl->flex_grow(grow); }
    void flex_shrink(float shrink) { m_impl->flex_shrink(shrink); }
    void flex_wrap(layout_flex_wrap wrap) { m_impl->flex_wrap(wrap); }
    void justify_content(layout_justify justify) { m_impl->justify_content(justify); }
    void align_items(layout_align align) { m_impl->align_items(align); }
    void align_self(layout_align align) { m_impl->align_self(align); }
    void align_content(layout_align align) { m_impl->align_content(align); }
    void padding(float padding, layout_edge edge) { m_impl->padding(padding, edge); }
    void border(float border, layout_edge edge) { m_impl->border(border, edge); }
    void margin(float margin, layout_edge edge) { m_impl->margin(margin, edge); }
    virtual void position_type(layout_position_type position_type)
    {
        return m_impl->position_type(position_type);
    }
    virtual void position(float position, layout_edge edge, bool percent = false)
    {
        m_impl->position(position, edge, percent);
    }

    layout_direction direction() const { return m_impl->direction(); }
    layout_flex_direction flex_direction() const { return m_impl->flex_direction(); }
    float flex_basis() const { return m_impl->flex_basis(); }
    float flex_grow() const { return m_impl->flex_grow(); }
    float flex_shrink() const { return m_impl->flex_shrink(); }
    layout_flex_wrap flex_wrap() const { return m_impl->flex_wrap(); }
    layout_justify justify_content() const { return m_impl->justify_content(); }
    layout_align align_items() const { return m_impl->align_items(); }
    layout_align align_self() const { return m_impl->align_self(); }
    layout_align align_content() const { return m_impl->align_content(); }
    float padding(layout_edge edge) { return m_impl->padding(edge); }
    float border(layout_edge edge) { return m_impl->border(edge); }
    float margin(layout_edge edge) { return m_impl->margin(edge); }

    void calculate(float width, float height) { m_impl->calculate(width, height); }
    void calculate_absolute_position(float parent_x, float parent_y)
    {
        m_impl->calculate_absolute_position(parent_x, parent_y);
    }

    void width(float value) { return m_impl->width(value); }
    void width_auto() { return m_impl->width_auto(); }
    void width_percent(float value) { return m_impl->width_percent(value); }
    void width_min(float value) { return m_impl->width_min(value); }
    void width_max(float value) { return m_impl->width_max(value); }

    void height(float value) { return m_impl->height(value); }
    void height_auto() { return m_impl->height_auto(); }
    void height_percent(float value) { return m_impl->height_percent(value); }
    void height_min(float value) { return m_impl->height_min(value); }
    void height_max(float value) { return m_impl->height_max(value); }

    void copy_style(element_layout* target) { m_impl->copy_style(target->m_impl.get()); }

    bool layout_dirty() const { return m_impl->dirty(); }

protected:
    void layout_add_child(element_layout* child, std::size_t index)
    {
        m_impl->add_child(child->m_impl.get(), index);
    }
    void layout_remove_child(element_layout* child) { m_impl->remove_child(child->m_impl.get()); }

    element_extent layout_extent() const { return m_impl->extent(); }
    void layout_display(bool display) { m_impl->display(display); }

private:
    std::unique_ptr<layout_node_impl> m_impl;
};
} // namespace ash::ui