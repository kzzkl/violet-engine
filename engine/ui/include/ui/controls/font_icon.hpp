#pragma once

#include "ui/color.hpp"
#include "ui/element.hpp"

namespace ash::ui
{
class font;

struct font_icon_theme
{
    const font* icon_font;
    std::uint32_t icon_color;
    float icon_scale;
};

class font_icon : public element
{
public:
    font_icon(std::uint32_t index, const font_icon_theme& theme);

    void icon(std::uint32_t index);
    void icon_scale(float scale);
    void icon_color(std::uint32_t color);

    virtual void render(renderer& renderer) override;

protected:
    virtual void on_extent_change(const element_extent& extent) override;

private:
    const font* m_font;

    float m_icon_scale;
    float m_icon_width;
    float m_icon_height;
};
} // namespace ash::ui