#pragma once

#include "ui/color.hpp"
#include "ui/element.hpp"
#include "ui/font.hpp"

namespace ash::ui
{
class label : public element
{
public:
    label();
    label(std::string_view content, const font& font, std::uint32_t color = COLOR_BLACK);

    void text(std::string_view content, const font& font);
    void text_color(std::uint32_t color);

    std::string text() const noexcept { return m_text; }

    virtual void render(renderer& renderer) override;

protected:
    virtual void on_extent_change() override;

private:
    std::string m_text;
    std::uint32_t m_text_color;

    float m_original_x;
    float m_original_y;
};
} // namespace ash::ui