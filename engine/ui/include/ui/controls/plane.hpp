#pragma once

#include "ui/element_control.hpp"

namespace ash::ui
{
class plane : public element_control
{
public:
    plane(std::uint32_t color);
    virtual void extent(const element_extent& extent) override;
};
} // namespace ash::ui