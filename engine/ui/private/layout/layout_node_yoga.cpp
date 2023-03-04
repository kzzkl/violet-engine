#include "layout/layout_node_yoga.hpp"
#include "common/assert.hpp"

namespace violet::ui
{
namespace
{
template <typename T>
struct translate_unit
{
};

template <typename T>
struct translate
{
    using unit_type = translate_unit<T>;

    using yoga_type = std::decay_t<decltype(unit_type::yoga_map[0])>;
    using internal_type = T;

    static yoga_type to_yoga(internal_type value) { return unit_type::yoga_map[value]; }
    static internal_type to_internal(yoga_type value) { return unit_type::internal_map[value]; }
};

template <>
struct translate_unit<layout_direction>
{
    static constexpr YGDirection yoga_map[] = {YGDirectionInherit, YGDirectionLTR, YGDirectionRTL};
    static constexpr layout_direction internal_map[] = {
        LAYOUT_DIRECTION_INHERIT,
        LAYOUT_DIRECTION_LTR,
        LAYOUT_DIRECTION_RTL};
};

template <>
struct translate_unit<layout_flex_direction>
{
    static constexpr YGFlexDirection yoga_map[] = {
        YGFlexDirectionColumn,
        YGFlexDirectionColumnReverse,
        YGFlexDirectionRow,
        YGFlexDirectionRowReverse};
    static constexpr layout_flex_direction internal_map[] = {
        LAYOUT_FLEX_DIRECTION_COLUMN,
        LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE,
        LAYOUT_FLEX_DIRECTION_ROW,
        LAYOUT_FLEX_DIRECTION_ROW_REVERSE};
};

template <>
struct translate_unit<layout_flex_wrap>
{
    static constexpr YGWrap yoga_map[] = {YGWrapNoWrap, YGWrapWrap, YGWrapWrapReverse};
    static constexpr layout_flex_wrap internal_map[] = {
        LAYOUT_FLEX_WRAP_NOWRAP,
        LAYOUT_FLEX_WRAP_WRAP,
        LAYOUT_FLEX_WRAP_WRAP_REVERSE};
};

template <>
struct translate_unit<layout_justify>
{
    static constexpr YGJustify yoga_map[] = {
        YGJustifyFlexStart,
        YGJustifyCenter,
        YGJustifyFlexEnd,
        YGJustifySpaceBetween,
        YGJustifySpaceAround,
        YGJustifySpaceEvenly};
    static constexpr layout_justify internal_map[] = {
        LAYOUT_JUSTIFY_FLEX_START,
        LAYOUT_JUSTIFY_CENTER,
        LAYOUT_JUSTIFY_FLEX_END,
        LAYOUT_JUSTIFY_SPACE_BETWEEN,
        LAYOUT_JUSTIFY_SPACE_AROUND,
        LAYOUT_JUSTIFY_SPACE_EVENLY};
};

template <>
struct translate_unit<layout_align>
{
    static constexpr YGAlign yoga_map[] = {
        YGAlignAuto,
        YGAlignFlexStart,
        YGAlignCenter,
        YGAlignFlexEnd,
        YGAlignStretch,
        YGAlignBaseline,
        YGAlignSpaceBetween,
        YGAlignSpaceAround};
    static constexpr layout_align internal_map[] = {
        LAYOUT_ALIGN_AUTO,
        LAYOUT_ALIGN_FLEX_START,
        LAYOUT_ALIGN_CENTER,
        LAYOUT_ALIGN_FLEX_END,
        LAYOUT_ALIGN_STRETCH,
        LAYOUT_ALIGN_BASELINE,
        LAYOUT_ALIGN_SPACE_BETWEEN,
        LAYOUT_ALIGN_SPACE_AROUND};
};

template <>
struct translate_unit<layout_edge>
{
    static constexpr YGEdge yoga_map[] = {
        YGEdgeLeft,
        YGEdgeTop,
        YGEdgeRight,
        YGEdgeBottom,
        YGEdgeStart,
        YGEdgeEnd,
        YGEdgeHorizontal,
        YGEdgeVertical,
        YGEdgeAll};
    static constexpr layout_edge internal_map[] = {
        LAYOUT_EDGE_LEFT,
        LAYOUT_EDGE_TOP,
        LAYOUT_EDGE_RIGHT,
        LAYOUT_EDGE_BOTTOM,
        LAYOUT_EDGE_START,
        LAYOUT_EDGE_END,
        LAYOUT_EDGE_HORIZONTAL,
        LAYOUT_EDGE_VERTICAL,
        LAYOUT_EDGE_ALL};
};
template <>
struct translate_unit<layout_position_type>
{
    static constexpr YGPositionType yoga_map[] = {
        YGPositionTypeStatic,
        YGPositionTypeRelative,
        YGPositionTypeAbsolute};
    static constexpr layout_position_type internal_map[] = {
        LAYOUT_POSITION_TYPE_STATIC,
        LAYOUT_POSITION_TYPE_RELATIVE,
        LAYOUT_POSITION_TYPE_ABSOLUTE};
};
} // namespace

layout_node_yoga::layout_node_yoga(bool make_node) : m_absolute_x(0.0), m_absolute_y(0.0)
{
    if (make_node)
        m_node = YGNodeNew();
}

void layout_node_yoga::set_direction(layout_direction direction)
{
    YGNodeStyleSetDirection(m_node, translate<layout_direction>::to_yoga(direction));
}

void layout_node_yoga::set_flex_direction(layout_flex_direction flex_direction)
{
    YGNodeStyleSetFlexDirection(m_node, translate<layout_flex_direction>::to_yoga(flex_direction));
}

void layout_node_yoga::set_flex_basis(float basis)
{
    YGNodeStyleSetFlexBasis(m_node, basis);
}

void layout_node_yoga::set_flex_grow(float grow)
{
    YGNodeStyleSetFlexGrow(m_node, grow);
}

void layout_node_yoga::set_flex_shrink(float shrink)
{
    YGNodeStyleSetFlexShrink(m_node, shrink);
}

void layout_node_yoga::set_flex_wrap(layout_flex_wrap wrap)
{
    YGNodeStyleSetFlexWrap(m_node, translate<layout_flex_wrap>::to_yoga(wrap));
}

void layout_node_yoga::set_justify_content(layout_justify justify)
{
    YGNodeStyleSetJustifyContent(m_node, translate<layout_justify>::to_yoga(justify));
}

void layout_node_yoga::set_align_items(layout_align align)
{
    YGNodeStyleSetAlignItems(m_node, translate<layout_align>::to_yoga(align));
}

void layout_node_yoga::set_align_self(layout_align align)
{
    YGNodeStyleSetAlignSelf(m_node, translate<layout_align>::to_yoga(align));
}

void layout_node_yoga::set_align_content(layout_align align)
{
    YGNodeStyleSetAlignContent(m_node, translate<layout_align>::to_yoga(align));
}

void layout_node_yoga::set_padding(float padding, layout_edge edge)
{
    YGNodeStyleSetPadding(m_node, translate<layout_edge>::to_yoga(edge), padding);
}

void layout_node_yoga::set_border(float border, layout_edge edge)
{
    YGNodeStyleSetBorder(m_node, translate<layout_edge>::to_yoga(edge), border);
}

void layout_node_yoga::set_margin(float margin, layout_edge edge)
{
    YGNodeStyleSetMargin(m_node, translate<layout_edge>::to_yoga(edge), margin);
}

void layout_node_yoga::set_display(bool display)
{
    if (display)
        YGNodeStyleSetDisplay(m_node, YGDisplayFlex);
    else
        YGNodeStyleSetDisplay(m_node, YGDisplayNone);
}

void layout_node_yoga::set_position_type(layout_position_type position_type)
{
    YGNodeStyleSetPositionType(m_node, translate<layout_position_type>::to_yoga(position_type));
}

void layout_node_yoga::set_position(float position, layout_edge edge, bool percent)
{
    if (percent)
        YGNodeStyleSetPositionPercent(m_node, translate<layout_edge>::to_yoga(edge), position);
    else
        YGNodeStyleSetPosition(m_node, translate<layout_edge>::to_yoga(edge), position);
}

void layout_node_yoga::set_width(float value)
{
    YGNodeStyleSetWidth(m_node, value);
}

void layout_node_yoga::set_width_auto()
{
    YGNodeStyleSetWidthAuto(m_node);
}

void layout_node_yoga::set_width_percent(float value)
{
    YGNodeStyleSetWidthPercent(m_node, value);
}

void layout_node_yoga::set_width_min(float value)
{
    YGNodeStyleSetMinWidth(m_node, value);
}

void layout_node_yoga::set_width_max(float value)
{
    YGNodeStyleSetMaxWidth(m_node, value);
}

void layout_node_yoga::set_height(float value)
{
    YGNodeStyleSetHeight(m_node, value);
}

void layout_node_yoga::set_height_auto()
{
    YGNodeStyleSetHeightAuto(m_node);
}

void layout_node_yoga::set_height_percent(float value)
{
    YGNodeStyleSetHeightPercent(m_node, value);
}

void layout_node_yoga::set_height_min(float value)
{
    YGNodeStyleSetMinHeight(m_node, value);
}

void layout_node_yoga::set_height_max(float value)
{
    YGNodeStyleSetMaxHeight(m_node, value);
}

layout_direction layout_node_yoga::get_direction() const
{
    return translate<layout_direction>::to_internal(YGNodeStyleGetDirection(m_node));
}

layout_flex_direction layout_node_yoga::get_flex_direction() const
{
    return translate<layout_flex_direction>::to_internal(YGNodeStyleGetFlexDirection(m_node));
}

float layout_node_yoga::get_flex_basis() const
{
    return YGNodeStyleGetFlexBasis(m_node).value;
}

float layout_node_yoga::get_flex_grow() const
{
    return YGNodeStyleGetFlexGrow(m_node);
}

float layout_node_yoga::get_flex_shrink() const
{
    return YGNodeStyleGetFlexShrink(m_node);
}

layout_flex_wrap layout_node_yoga::get_flex_wrap() const
{
    return translate<layout_flex_wrap>::to_internal(YGNodeStyleGetFlexWrap(m_node));
}

layout_justify layout_node_yoga::get_justify_content() const
{
    return translate<layout_justify>::to_internal(YGNodeStyleGetJustifyContent(m_node));
}

layout_align layout_node_yoga::get_align_items() const
{
    return translate<layout_align>::to_internal(YGNodeStyleGetAlignItems(m_node));
}

layout_align layout_node_yoga::get_align_self() const
{
    return translate<layout_align>::to_internal(YGNodeStyleGetAlignSelf(m_node));
}

layout_align layout_node_yoga::get_align_content() const
{
    return translate<layout_align>::to_internal(YGNodeStyleGetAlignContent(m_node));
}

float layout_node_yoga::get_padding(layout_edge edge) const
{
    return YGNodeStyleGetPadding(m_node, translate<layout_edge>::to_yoga(edge)).value;
}

float layout_node_yoga::get_border(layout_edge edge) const
{
    return YGNodeStyleGetBorder(m_node, translate<layout_edge>::to_yoga(edge));
}

float layout_node_yoga::get_margin(layout_edge edge) const
{
    return YGNodeStyleGetMargin(m_node, translate<layout_edge>::to_yoga(edge)).value;
}

void layout_node_yoga::calculate(float width, float height)
{
    YGNodeCalculateLayout(m_node, width, height, YGDirectionLTR);
}

void layout_node_yoga::calculate_absolute_position(float parent_x, float parent_y)
{
    m_absolute_x = parent_x + YGNodeLayoutGetLeft(m_node);
    m_absolute_y = parent_y + YGNodeLayoutGetTop(m_node);
}

node_rect layout_node_yoga::get_rect() const
{
    return node_rect{
        .x = m_absolute_x,
        .y = m_absolute_y,
        .width = YGNodeLayoutGetWidth(m_node),
        .height = YGNodeLayoutGetHeight(m_node)};
}

bool layout_node_yoga::dirty() const
{
    return YGNodeIsDirty(m_node);
}

void layout_node_yoga::add_child(layout_node* child, std::size_t index)
{
    auto yoga_child = static_cast<layout_node_yoga*>(child);
    YGNodeInsertChild(m_node, yoga_child->m_node, static_cast<std::uint32_t>(index));
}

void layout_node_yoga::remove_child(layout_node* child)
{
    auto yoga_child = static_cast<layout_node_yoga*>(child);
    VIOLET_ASSERT(YGNodeGetParent(yoga_child->m_node) == m_node);
    YGNodeRemoveChild(m_node, yoga_child->m_node);
}

layout_root_yoga::layout_root_yoga() : layout_node_yoga(false)
{
    m_config = YGConfigNew();
    m_node = YGNodeNewWithConfig(m_config);
}

void layout_root_yoga::calculate(float width, float height)
{
    YGNodeCalculateLayout(m_node, width, height, YGDirectionLTR);
}
} // namespace violet::ui