#pragma once

#include "ui/color.hpp"
#include "ui/element.hpp"
#include "ui/font.hpp"

namespace ash::ui
{
struct font_icon_style
{
    const font* icon_font;
    std::uint32_t icon_color{COLOR_BLACK};
    float icon_scale{1.0f};
};

class font_icon : public element
{
public:
    font_icon(std::uint32_t index, const font_icon_style& style);

    void icon(std::uint32_t index, const font& font);
    void icon_scale(float scale);
    void icon_color(std::uint32_t color);

    virtual void render(renderer& renderer) override;

protected:
    virtual void on_extent_change(const element_extent& extent) override;

private:
    float m_icon_scale;
    float m_icon_width;
    float m_icon_height;
};
} // namespace ash::ui