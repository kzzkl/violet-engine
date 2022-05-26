#pragma once

#include "ui/controls/plane.hpp"

namespace ash::ui
{
class image : public plane
{
public:
    image(graphics::resource* texture);

    void texture(graphics::resource* texture);
};
} // namespace ash::ui