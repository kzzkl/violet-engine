#pragma once

#include "ui/layout/layout.hpp"
#include "yoga/Yoga.h"

namespace violet
{
class widget_layout_yoga : public widget_layout
{
public:
    widget_layout_yoga(bool make_node = true);

    virtual void set_direction(layout_direction direction) override;
    virtual layout_direction get_direction() const override;

    virtual void set_flex_direction(layout_flex_direction flex_direction) override;
    virtual layout_flex_direction get_flex_direction() const override;

    virtual void set_flex_basis(float basis) override;
    virtual float get_flex_basis() const override;

    virtual void set_flex_grow(float grow) override;
    virtual float get_flex_grow() const override;

    virtual void set_flex_shrink(float shrink) override;
    virtual float get_flex_shrink() const override;

    virtual void set_flex_wrap(layout_flex_wrap wrap) override;
    virtual layout_flex_wrap get_flex_wrap() const override;

    virtual void set_justify_content(layout_justify justify) override;
    virtual layout_justify get_justify_content() const override;

    virtual void set_align_items(layout_align align) override;
    virtual layout_align get_align_items() const override;

    virtual void set_align_self(layout_align align) override;
    virtual layout_align get_align_self() const override;

    virtual void set_align_content(layout_align align) override;
    virtual layout_align get_align_content() const override;

    virtual void set_padding(float padding, layout_edge edge) override;
    virtual float get_padding(layout_edge edge) const override;

    virtual void set_border(float border, layout_edge edge) override;
    virtual float get_border(layout_edge edge) const override;

    virtual void set_margin(float margin, layout_edge edge) override;
    virtual float get_margin(layout_edge edge) const override;

    virtual void set_display(bool display) override;

    virtual void set_position_type(layout_position_type position_type) override;
    virtual void set_position(float position, layout_edge edge, bool percent) override;

    virtual void set_width(float value) override;
    virtual void set_width_auto() override;
    virtual void set_width_percent(float value) override;
    virtual void set_width_min(float value) override;
    virtual void set_width_max(float value) override;

    virtual void set_height(float value) override;
    virtual void set_height_auto() override;
    virtual void set_height_percent(float value) override;
    virtual void set_height_min(float value) override;
    virtual void set_height_max(float value) override;

    virtual void calculate(float width, float height) override;
    virtual void calculate_absolute_position(float parent_x, float parent_y) override;

    virtual bool has_updated_flag() override;
    virtual void reset_updated_flag() override;

    virtual std::uint32_t get_x() const override;
    virtual std::uint32_t get_y() const override;
    virtual std::uint32_t get_absolute_x() const override
    {
        return static_cast<std::uint32_t>(m_absolute_x);
    }
    virtual std::uint32_t get_absolute_y() const override
    {
        return static_cast<std::uint32_t>(m_absolute_y);
    }
    virtual std::uint32_t get_width() const override;
    virtual std::uint32_t get_height() const override;

    virtual bool dirty() const override;

    virtual void add_child(widget_layout* child, std::size_t index) override;
    virtual void remove_child(widget_layout* child) override;

protected:
    YGNodeRef m_node;

    float m_absolute_x;
    float m_absolute_y;
};

class widget_layout_root_yoga : public widget_layout_yoga
{
public:
    widget_layout_root_yoga();

    void calculate(float width, float height);

private:
    YGConfigRef m_config;
};
} // namespace violet