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

    void slider_size(float size);

    std::function<void(float)> on_slide;

private:
    bool m_vertical;

    float m_position;
    std::unique_ptr<panel> m_slider;
};

struct scroll_view_style
{
    float scroll_speed{30.0f};

    float bar_width{8.0f};
    std::uint32_t bar_color{COLOR_WHITE};
    std::uint32_t slider_color{0xFFAB938D};
    std::uint32_t background_color{COLOR_WHITE};
};

class scroll_view : public view_panel
{
public:
    scroll_view(const scroll_view_style& style = {});

    void add(element* element);
    void remove(element* element);

protected:
    virtual void on_extent_change(const element_extent& extent) override;

private:
    void update_container_vertical_position(float scroll_value);
    void update_container_horizontal_position(float scroll_value);
    void update_scroll_bar(
        float view_width,
        float view_height,
        float container_width,
        float container_height);

    std::unique_ptr<scroll_bar> m_vertical_bar;
    std::unique_ptr<scroll_bar> m_horizontal_bar;

    std::unique_ptr<element> m_container;
};
} // namespace ash::ui