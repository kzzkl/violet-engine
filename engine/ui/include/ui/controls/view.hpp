#pragma once

#include "ui/element.hpp"

namespace ash::ui
{
class view : public element
{
public:
    view();

    virtual void render(renderer& renderer) override;
};
} // namespace ash::ui