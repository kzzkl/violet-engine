#pragma once

#include "ui/element.hpp"

namespace ash::ui
{
class image : public element
{
public:
    image(graphics::resource* texture = nullptr);

    void texture(graphics::resource* texture, bool resize = false);

    virtual void render(renderer& renderer) override;

protected:
    virtual void on_extent_change() override;
};
} // namespace ash::ui