#pragma once

#include "ui/controls/panel.hpp"
#include "ui/controls/view.hpp"

namespace ash::ui
{
class scroll_bar : public panel
{
public:
    scroll_bar(
        bool vertical = true,
        std::uint32_t slider_color = COLOR_WHITE,
        std::uint32_t bar_color = COLOR_GRAY);

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
    scroll_bar* m_vertical_bar;
    scroll_bar* m_horizontal_bar;
};

class scroll_view : public panel
{
public:
    scroll_view();

    element* container() const noexcept { return m_container.get(); }

private:
    void sync_container_vertical_position(float bar_value);
    void sync_container_horizontal_position(float bar_value);

    std::unique_ptr<element> m_layout_left;

    std::unique_ptr<scroll_bar> m_vertical_bar;
    std::unique_ptr<scroll_bar> m_horizontal_bar;

    std::unique_ptr<view> m_container_view;
    std::unique_ptr<scroll_container> m_container;
};
} // namespace ash::ui