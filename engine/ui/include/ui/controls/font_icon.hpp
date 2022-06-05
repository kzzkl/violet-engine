#pragma once

#include "ui/color.hpp"
#include "ui/element.hpp"
#include "ui/font.hpp"

namespace ash::ui
{
class font_icon : public element
{
public:
    font_icon(
        std::uint32_t index,
        const font& font,
        float scale = 1.0f,
        std::uint32_t color = COLOR_BLACK);

    void icon(std::uint32_t index, const font& font);
    void icon_scale(float scale);
    void icon_color(std::uint32_t color);

    virtual void render(renderer& renderer) override;

protected:
    virtual void on_extent_change() override;

private:
    float m_icon_scale;
    float m_icon_width;
    float m_icon_height;
    std::uint32_t m_icon_color;
};
} // namespace ash::ui