#pragma once

#include "ui/color.hpp"
#include "ui/element_control.hpp"

namespace ash::ui
{
class plane : public element_control
{
public:
    plane(std::uint32_t color = COLOR_WHITE);
    virtual void extent(const element_extent& extent) override;
};
} // namespace ash::ui