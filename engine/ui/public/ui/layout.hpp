#pragma once

#include "math/rect.hpp"
#include <cstddef>
#include <cstdint>

namespace violet::ui
{
enum layout_direction : std::uint8_t
{
    LAYOUT_DIRECTION_INHERIT,
    LAYOUT_DIRECTION_LTR,
    LAYOUT_DIRECTION_RTL
};

enum layout_flex_direction : std::uint8_t
{
    LAYOUT_FLEX_DIRECTION_COLUMN,
    LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE,
    LAYOUT_FLEX_DIRECTION_ROW,
    LAYOUT_FLEX_DIRECTION_ROW_REVERSE
};

enum layout_flex_wrap : std::uint8_t
{
    LAYOUT_FLEX_WRAP_NOWRAP,
    LAYOUT_FLEX_WRAP_WRAP,
    LAYOUT_FLEX_WRAP_WRAP_REVERSE
};

enum layout_justify : std::uint8_t
{
    LAYOUT_JUSTIFY_FLEX_START,
    LAYOUT_JUSTIFY_CENTER,
    LAYOUT_JUSTIFY_FLEX_END,
    LAYOUT_JUSTIFY_SPACE_BETWEEN,
    LAYOUT_JUSTIFY_SPACE_AROUND,
    LAYOUT_JUSTIFY_SPACE_EVENLY
};

enum layout_align : std::uint8_t
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

enum layout_edge : std::uint8_t
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

enum layout_position_type : std::uint8_t
{
    LAYOUT_POSITION_TYPE_STATIC,
    LAYOUT_POSITION_TYPE_RELATIVE,
    LAYOUT_POSITION_TYPE_ABSOLUTE
};

using node_rect = math::rect<float>;

class layout_node
{
public:
    virtual ~layout_node() = default;

    virtual void set_direction(layout_direction direction) = 0;
    virtual void set_flex_direction(layout_flex_direction flex_direction) = 0;
    virtual void set_flex_basis(float basis) = 0;
    virtual void set_flex_grow(float grow) = 0;
    virtual void set_flex_shrink(float shrink) = 0;
    virtual void set_flex_wrap(layout_flex_wrap wrap) = 0;
    virtual void set_justify_content(layout_justify justify) = 0;
    virtual void set_align_items(layout_align align) = 0;
    virtual void set_align_self(layout_align align) = 0;
    virtual void set_align_content(layout_align align) = 0;
    virtual void set_padding(float padding, layout_edge edge) = 0;
    virtual void set_border(float border, layout_edge edge) = 0;
    virtual void set_margin(float margin, layout_edge edge) = 0;
    virtual void set_display(bool display) = 0;
    virtual void set_position_type(layout_position_type position_type) = 0;
    virtual void set_position(float position, layout_edge edge, bool percent = false) = 0;
    virtual void set_width(float value) = 0;
    virtual void set_width_auto() = 0;
    virtual void set_width_percent(float value) = 0;
    virtual void set_width_min(float value) = 0;
    virtual void set_width_max(float value) = 0;
    virtual void set_height(float value) = 0;
    virtual void set_height_auto() = 0;
    virtual void set_height_percent(float value) = 0;
    virtual void set_height_min(float value) = 0;
    virtual void set_height_max(float value) = 0;

    virtual layout_direction get_direction() const = 0;
    virtual layout_flex_direction get_flex_direction() const = 0;
    virtual float get_flex_basis() const = 0;
    virtual float get_flex_grow() const = 0;
    virtual float get_flex_shrink() const = 0;
    virtual layout_flex_wrap get_flex_wrap() const = 0;
    virtual layout_justify get_justify_content() const = 0;
    virtual layout_align get_align_items() const = 0;
    virtual layout_align get_align_self() const = 0;
    virtual layout_align get_align_content() const = 0;
    virtual float get_padding(layout_edge edge) const = 0;
    virtual float get_border(layout_edge edge) const = 0;
    virtual float get_margin(layout_edge edge) const = 0;

    virtual void calculate(float width, float height) = 0;
    virtual void calculate_absolute_position(float parent_x, float parent_y) = 0;

    virtual node_rect get_rect() const = 0;
    virtual bool dirty() const = 0;

    virtual void add_child(layout_node* child, std::size_t index) = 0;
    virtual void remove_child(layout_node* child) = 0;
};
} // namespace violet::ui