#pragma once

#include "ui/color.hpp"
#include "ui/element.hpp"

namespace ash::ui
{
class font;

struct label_theme
{
    const font* text_font;
    std::uint32_t text_color;
};

class label : public element
{
public:
    label();
    label(std::string_view content, const label_theme& theme);

    void text(std::string_view content);
    void text_color(std::uint32_t color);

    std::string text() const noexcept { return m_text; }

    virtual void render(renderer& renderer) override;

protected:
    virtual void on_extent_change(const element_extent& extent) override;

private:
    std::string m_text;
    const font* m_font;

    float m_original_x;
    float m_original_y;
};
} // namespace ash::ui