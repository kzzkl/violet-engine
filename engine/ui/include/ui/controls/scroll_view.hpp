#pragma once

#include "ui/controls/panel.hpp"
#include "ui/controls/view.hpp"

namespace ash::ui
{
class scroll_bar : public panel
{
public:
    scroll_bar(bool vertical, std::uint32_t slider_color, std::uint32_t bar_color);

    void value(float v) noexcept;
    float value() const noexcept;
    element* slider() const noexcept { return m_slider.get(); }

    std::function<void(float)> on_slide;

private:
    bool m_vertical;

    float m_position;
    std::unique_ptr<panel> m_slider;
};

class scroll_container : public element
{
public:
    scroll_container(scroll_bar* vertical_bar, scroll_bar* horizontal_bar);

protected:
    virtual void on_extent_change() override;

private:
    float m_width;
    float m_height;

    scroll_bar* m_vertical_bar;
    scroll_bar* m_horizontal_bar;
};

struct scroll_view_style
{
    float scroll_speed;

    float bar_width;
    std::uint32_t bar_color;
    std::uint32_t slider_color;

    std::uint32_t background_color;
};

class scroll_view : public panel
{
public:
    static constexpr scroll_view_style default_style = {
        .scroll_speed = 30.0f,
        .bar_width = 8.0f,
        .bar_color = COLOR_WHITE,
        .slider_color = 0xFFAB938D,
        .background_color = COLOR_WHITE};

public:
    scroll_view(const scroll_view_style& style = default_style);

    void add(element* element);
    void remove(element* element);

    element* container() const noexcept { return m_container.get(); }

private:
    void sync_container_vertical_position(float bar_value);
    void sync_container_horizontal_position(float bar_value);

    std::unique_ptr<element> m_left;

    std::unique_ptr<scroll_bar> m_vertical_bar;
    std::unique_ptr<scroll_bar> m_horizontal_bar;

    std::unique_ptr<view> m_container_view;
    std::unique_ptr<scroll_container> m_container;
};
} // namespace ash::ui