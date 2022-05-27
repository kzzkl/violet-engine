#pragma once

#include "ui/color.hpp"
#include "ui/element.hpp"

namespace ash::ui
{
class panel : public element
{
public:
    panel(std::uint32_t color = COLOR_WHITE);

    virtual void render(renderer& renderer) override;

public:
    virtual void on_extent_change(const element_extent& extent) override;
};
} // namespace ash::ui