#pragma once

#include "ui/color.hpp"
#include "ui/element.hpp"
#include "ui/font.hpp"

namespace ash::ui
{
class label : public element
{
public:
    label(std::string_view text, const font& font, std::uint32_t color = COLOR_BLACK);

    void text(std::string_view text, const font& font, std::uint32_t color = COLOR_BLACK);
    std::string_view text() const noexcept { return m_text; }

    virtual void render(renderer& renderer) override;

protected:
    virtual void on_extent_change() override;

private:
    std::string m_text;

    float m_original_x;
    float m_original_y;
};
} // namespace ash::ui