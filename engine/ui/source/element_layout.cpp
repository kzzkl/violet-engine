#include "ui/element_layout.hpp"
#include <yoga/Yoga.h>

namespace ash::ui
{
class layout_node_impl_yoga : public layout_node_impl
{
private:
    static constexpr YGDirection INTERNAL_YOGA_DIRECTION_MAP[] = {
        YGDirectionInherit,
        YGDirectionLTR,
        YGDirectionRTL};
    static constexpr YGFlexDirection INTERNAL_YOGA_FLEX_DIRECTION_MAP[] = {
        YGFlexDirectionColumn,
        YGFlexDirectionColumnReverse,
        YGFlexDirectionRow,
        YGFlexDirectionRowReverse};
    static constexpr YGWrap INTERNAL_YOGA_FLEX_WRAP_MAP[] = {
        YGWrapNoWrap,
        YGWrapWrap,
        YGWrapWrapReverse};
    static constexpr YGJustify INTERNAL_YOGA_JUSTIFY_MAP[] = {
        YGJustifyFlexStart,
        YGJustifyCenter,
        YGJustifyFlexEnd,
        YGJustifySpaceBetween,
        YGJustifySpaceAround,
        YGJustifySpaceEvenly};
    static constexpr YGAlign INTERNAL_YOGA_ALIGN_MAP[] = {
        YGAlignAuto,
        YGAlignFlexStart,
        YGAlignCenter,
        YGAlignFlexEnd,
        YGAlignStretch,
        YGAlignBaseline,
        YGAlignSpaceBetween,
        YGAlignSpaceAround};
    static constexpr YGEdge INTERNAL_YOGA_EDGE_MAP[] = {
        YGEdgeLeft,
        YGEdgeTop,
        YGEdgeRight,
        YGEdgeBottom,
        YGEdgeStart,
        YGEdgeEnd,
        YGEdgeHorizontal,
        YGEdgeVertical,
        YGEdgeAll};

    static constexpr layout_direction YOGA_INTERNAL_DIRECTION_MAP[] = {
        LAYOUT_DIRECTION_INHERIT,
        LAYOUT_DIRECTION_LTR,
        LAYOUT_DIRECTION_RTL};
    static constexpr layout_flex_direction YOGA_INTERNAL_FLEX_DIRECTION_MAP[] = {
        LAYOUT_FLEX_DIRECTION_COLUMN,
        LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE,
        LAYOUT_FLEX_DIRECTION_ROW,
        LAYOUT_FLEX_DIRECTION_ROW_REVERSE};
    static constexpr layout_flex_wrap YOGA_INTERNAL_FLEX_WRAP_MAP[] = {
        LAYOUT_FLEX_WRAP_NOWRAP,
        LAYOUT_FLEX_WRAP_WRAP,
        LAYOUT_FLEX_WRAP_WRAP_REVERSE};
    static constexpr layout_justify YOGA_INTERNAL_JUSTIFY_MAP[] = {
        LAYOUT_JUSTIFY_FLEX_START,
        LAYOUT_JUSTIFY_CENTER,
        LAYOUT_JUSTIFY_FLEX_END,
        LAYOUT_JUSTIFY_SPACE_BETWEEN,
        LAYOUT_JUSTIFY_SPACE_AROUND,
        LAYOUT_JUSTIFY_SPACE_EVENLY};
    static constexpr layout_align YOGA_INTERNAL_ALIGN_MAP[] = {
        LAYOUT_ALIGN_AUTO,
        LAYOUT_ALIGN_FLEX_START,
        LAYOUT_ALIGN_CENTER,
        LAYOUT_ALIGN_FLEX_END,
        LAYOUT_ALIGN_STRETCH,
        LAYOUT_ALIGN_BASELINE,
        LAYOUT_ALIGN_SPACE_BETWEEN,
        LAYOUT_ALIGN_SPACE_AROUND};
    static constexpr layout_edge YOGA_INTERNAL_EDGE_MAP[] = {
        LAYOUT_EDGE_LEFT,
        LAYOUT_EDGE_TOP,
        LAYOUT_EDGE_RIGHT,
        LAYOUT_EDGE_BOTTOM,
        LAYOUT_EDGE_START,
        LAYOUT_EDGE_END,
        LAYOUT_EDGE_HORIZONTAL,
        LAYOUT_EDGE_VERTICAL,
        LAYOUT_EDGE_ALL};

public:
    layout_node_impl_yoga(bool is_root)
        : m_config(nullptr),
          m_absolute_x(0.0f),
          m_absolute_y(0.0f),
          m_children_count(0)
    {
        if (is_root)
        {
            m_config = YGConfigNew();
            m_node = YGNodeNewWithConfig(m_config);
        }
        else
        {
            m_node = YGNodeNew();
        }
    }

    virtual void direction(layout_direction direction) override
    {
        YGNodeStyleSetDirection(m_node, INTERNAL_YOGA_DIRECTION_MAP[direction]);
    }
    virtual void flex_direction(layout_flex_direction flex_direction) override
    {
        YGNodeStyleSetFlexDirection(m_node, INTERNAL_YOGA_FLEX_DIRECTION_MAP[flex_direction]);
    }
    virtual void flex_basis(float basis) override { YGNodeStyleSetFlexBasis(m_node, basis); }
    virtual void flex_grow(float grow) override { YGNodeStyleSetFlexGrow(m_node, grow); }
    virtual void flex_shrink(float shrink) override { YGNodeStyleSetFlexShrink(m_node, shrink); }
    virtual void flex_wrap(layout_flex_wrap wrap) override
    {
        YGNodeStyleSetFlexWrap(m_node, INTERNAL_YOGA_FLEX_WRAP_MAP[wrap]);
    }
    virtual void justify(layout_justify justify) override
    {
        YGNodeStyleSetJustifyContent(m_node, INTERNAL_YOGA_JUSTIFY_MAP[justify]);
    }
    virtual void align_items(layout_align align) override
    {
        YGNodeStyleSetAlignItems(m_node, INTERNAL_YOGA_ALIGN_MAP[align]);
    }
    virtual void align_self(layout_align align) override
    {
        YGNodeStyleSetAlignSelf(m_node, INTERNAL_YOGA_ALIGN_MAP[align]);
    }
    virtual void align_content(layout_align align) override
    {
        YGNodeStyleSetAlignContent(m_node, INTERNAL_YOGA_ALIGN_MAP[align]);
    }
    virtual void padding(layout_edge edge, float padding) override
    {
        YGNodeStyleSetPadding(m_node, INTERNAL_YOGA_EDGE_MAP[edge], padding);
    }
    virtual void display(bool display) override
    {
        if (display)
            YGNodeStyleSetDisplay(m_node, YGDisplayFlex);
        else
            YGNodeStyleSetDisplay(m_node, YGDisplayNone);
    }

    virtual layout_direction direction() const override
    {
        return YOGA_INTERNAL_DIRECTION_MAP[YGNodeStyleGetDirection(m_node)];
    }
    virtual layout_flex_direction flex_direction() const override
    {
        return YOGA_INTERNAL_FLEX_DIRECTION_MAP[YGNodeStyleGetFlexDirection(m_node)];
    }
    virtual float flex_basis() const override { return YGNodeStyleGetFlexBasis(m_node).value; }
    virtual float flex_grow() const override { return YGNodeStyleGetFlexGrow(m_node); }
    virtual float flex_shrink() const override { return YGNodeStyleGetFlexShrink(m_node); }
    virtual layout_flex_wrap flex_wrap() const override
    {
        return YOGA_INTERNAL_FLEX_WRAP_MAP[YGNodeStyleGetFlexWrap(m_node)];
    }
    virtual layout_justify justify() const override
    {
        return YOGA_INTERNAL_JUSTIFY_MAP[YGNodeStyleGetJustifyContent(m_node)];
    }
    virtual layout_align align_items() const override
    {
        return YOGA_INTERNAL_ALIGN_MAP[YGNodeStyleGetAlignItems(m_node)];
    }
    virtual layout_align align_self() const override
    {
        return YOGA_INTERNAL_ALIGN_MAP[YGNodeStyleGetAlignSelf(m_node)];
    }
    virtual layout_align align_content() const override
    {
        return YOGA_INTERNAL_ALIGN_MAP[YGNodeStyleGetAlignContent(m_node)];
    }
    virtual float padding(layout_edge edge) const override
    {
        return YGNodeStyleGetPadding(m_node, INTERNAL_YOGA_EDGE_MAP[edge]).value;
    }

    virtual void parent(layout_node_impl* parent) override
    {
        auto yoga_parent = static_cast<layout_node_impl_yoga*>(parent);
        YGNodeInsertChild(yoga_parent->m_node, m_node, yoga_parent->m_children_count);
        ++yoga_parent->m_children_count;
    }

    void calculate(float width, float height) override
    {
        YGNodeCalculateLayout(m_node, width, height, YGDirectionLTR);
    }

    virtual void calculate_absolute_position(float parent_x, float parent_y) override
    {
        m_absolute_x = parent_x + YGNodeLayoutGetLeft(m_node);
        m_absolute_y = parent_y + YGNodeLayoutGetTop(m_node);
    }

    virtual void resize(
        float width,
        float height,
        bool auto_width,
        bool auto_height,
        bool percent_width,
        bool percent_height) override
    {
        if (auto_width)
            YGNodeStyleSetWidthAuto(m_node);
        else if (percent_width)
            YGNodeStyleSetWidthPercent(m_node, width);
        else
            YGNodeStyleSetWidth(m_node, width);

        if (auto_height)
            YGNodeStyleSetHeightAuto(m_node);
        else if (percent_height)
            YGNodeStyleSetHeightPercent(m_node, height);
        else
            YGNodeStyleSetHeight(m_node, height);
    }

    virtual element_extent extent() const override
    {
        return element_extent{
            .x = m_absolute_x,
            .y = m_absolute_y,
            .width = YGNodeLayoutGetWidth(m_node),
            .height = YGNodeLayoutGetHeight(m_node)};
    }

    virtual bool dirty() const override { return YGNodeIsDirty(m_node); }

private:
    YGNodeRef m_node;
    YGConfigRef m_config;

    float m_absolute_x;
    float m_absolute_y;

    std::uint32_t m_children_count;
};

element_layout::element_layout(bool is_root)
    : m_impl(std::make_unique<layout_node_impl_yoga>(is_root))
{
}
} // namespace ash::ui