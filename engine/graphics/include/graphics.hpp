#pragma once

#include "submodule.hpp"
#include "graphics_interface.hpp"

namespace ash::graphics
{
class graphics : public ash::core::submodule
{
public:
    graphics();

    virtual bool initialize(const ash::common::dictionary& config) override;

private:
};
} // namespace ash::graphics