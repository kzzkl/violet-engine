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

class layout_node_impl
{
public:
    virtual void direction(layout_direction direction) = 0;
    virtual void flex_direction(layout_flex_direction flex_direction) = 0;
    virtual void flex_basis(float basis) = 0;
    virtual void flex_grow(float grow) = 0;
    virtual void flex_shrink(float shrink) = 0;
    virtual void flex_wrap(layout_flex_wrap wrap) = 0;
    virtual void justify(layout_justify justify) = 0;
    virtual void align_items(layout_align align) = 0;
    virtual void align_self(layout_align align) = 0;
    virtual void align_content(layout_align align) = 0;
    virtual void padding(layout_edge edge, float padding) = 0;
    virtual void display(bool display) = 0;

    virtual layout_direction direction() const = 0;
    virtual layout_flex_direction flex_direction() const = 0;
    virtual float flex_basis() const = 0;
    virtual float flex_grow() const = 0;
    virtual float flex_shrink() const = 0;
    virtual layout_flex_wrap flex_wrap() const = 0;
    virtual layout_justify justify() const = 0;
    virtual layout_align align_items() const = 0;
    virtual layout_align align_self() const = 0;
    virtual layout_align align_content() const = 0;
    virtual float padding(layout_edge edge) const = 0;

    virtual void parent(layout_node_impl* parent) = 0;

    virtual void calculate(float width, float height) = 0;
    virtual void calculate_absolute_position(float parent_x, float parent_y) = 0;

    virtual void resize(
        float width,
        float height,
        bool auto_width,
        bool auto_height,
        bool percent_width,
        bool percent_height) = 0;
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
    void justify(layout_justify justify) { m_impl->justify(justify); }
    void align_items(layout_align align) { m_impl->align_items(align); }
    void align_self(layout_align align) { m_impl->align_self(align); }
    void align_content(layout_align align) { m_impl->align_content(align); }
    void padding(layout_edge edge, float padding) { m_impl->padding(edge, padding); }

    layout_direction direction() const { return m_impl->direction(); }
    layout_flex_direction flex_direction() const { return m_impl->flex_direction(); }
    float flex_basis() const { return m_impl->flex_basis(); }
    float flex_grow() const { return m_impl->flex_grow(); }
    float flex_shrink() const { return m_impl->flex_shrink(); }
    layout_flex_wrap flex_wrap() const { return m_impl->flex_wrap(); }
    layout_justify justify() const { return m_impl->justify(); }
    layout_align align_items() const { return m_impl->align_items(); }
    layout_align align_self() const { return m_impl->align_self(); }
    layout_align align_content() const { return m_impl->align_content(); }
    float padding(layout_edge edge) { return m_impl->padding(edge); }

    void calculate(float width, float height) { m_impl->calculate(width, height); }
    void calculate_absolute_position(float parent_x, float parent_y)
    {
        m_impl->calculate_absolute_position(parent_x, parent_y);
    }

    void resize(
        float width,
        float height,
        bool auto_width = false,
        bool auto_height = false,
        bool percent_width = false,
        bool percent_height = false)
    {
        m_impl->resize(width, height, auto_width, auto_height, percent_width, percent_height);
    }

    bool layout_dirty() const { return m_impl->dirty(); }

protected:
    void layout_parent(element_layout* parent)
    {
        m_impl->parent(parent == nullptr ? nullptr : parent->m_impl.get());
    }

    element_extent layout_extent() const { return m_impl->extent(); }
    void layout_display(bool display) { m_impl->display(display); }

private:
    std::unique_ptr<layout_node_impl> m_impl;
};
} // namespace ash::ui