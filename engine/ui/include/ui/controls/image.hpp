#pragma once

#include "ui/element.hpp"

namespace ash::ui
{
class image : public element
{
public:
    image(graphics::resource* texture);

    virtual void render(renderer& renderer) override;

    void texture(graphics::resource* texture);

public:
    virtual void on_extent_change(const element_extent& extent) override;
};
} // namespace ash::ui