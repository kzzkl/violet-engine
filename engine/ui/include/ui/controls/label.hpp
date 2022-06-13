#pragma once

#include "ui/color.hpp"
#include "ui/element.hpp"
#include "ui/font.hpp"

namespace ash::ui
{
struct label_style
{
    const font* text_font{nullptr};
    std::uint32_t text_color{COLOR_BLACK};
};

class label : public element
{
public:
    label();
    label(std::string_view content, const label_style& style);

    void text(std::string_view content, const font& font);
    void text_color(std::uint32_t color);

    std::string text() const noexcept { return m_text; }

    virtual void render(renderer& renderer) override;

protected:
    virtual void on_extent_change(const element_extent& extent) override;

private:
    std::string m_text;

    float m_original_x;
    float m_original_y;
};
} // namespace ash::ui