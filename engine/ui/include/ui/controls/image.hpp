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

protected:
    virtual void on_extent_change() override;
};
} // namespace ash::ui