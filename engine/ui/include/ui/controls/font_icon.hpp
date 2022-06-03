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

    void reset(
        std::uint32_t index,
        const font& font,
        float scale = 1.0f,
        std::uint32_t color = COLOR_BLACK);

    virtual void render(renderer& renderer) override;

protected:
    virtual void on_extent_change() override;

private:
    float m_width;
    float m_height;
};
} // namespace ash::ui