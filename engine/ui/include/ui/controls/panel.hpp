#pragma once

#include "ui/color.hpp"
#include "ui/element.hpp"

namespace ash::ui
{
class panel : public element
{
public:
    panel(std::uint32_t color = COLOR_WHITE);

    void color(std::uint32_t color) noexcept;

    virtual void render(renderer& renderer) override;

protected:
    virtual void on_extent_change(const element_extent& extent) override;
};

class view_panel : public panel
{
public:
    view_panel(std::uint32_t color = COLOR_WHITE);

    virtual void render(renderer& renderer) override;
};
} // namespace ash::ui