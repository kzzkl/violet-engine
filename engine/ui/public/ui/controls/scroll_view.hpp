#pragma once

#include "ui/controls/panel.hpp"

namespace violet::ui
{
class scroll_bar;

struct scroll_view_theme
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
    scroll_view(const scroll_view_theme& theme);

    void add_item(control* item);
    void remove_item(control* item);

protected:
    virtual void on_extent_change(float width, float height) override;

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

    std::unique_ptr<control> m_container;
};
} // namespace violet::ui