#pragma once

#include "ui/element_control.hpp"
#include "ui/font.hpp"

namespace ash::ui
{
class label : public element_control
{
public:
    label(std::string_view text, const font& font, std::uint32_t color);

    virtual void extent(const element_extent& extent) override;

private:
    float m_original_x;
    float m_original_y;
};
} // namespace ash::ui