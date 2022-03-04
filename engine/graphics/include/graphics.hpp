#pragma once

#include "graphics_exports.hpp"
#include "graphics_plugin.hpp"
#include "submodule.hpp"

namespace ash::graphics
{
class GRAPHICS_API graphics : public ash::core::submodule
{
public:
    graphics();

    virtual bool initialize(const ash::common::dictionary& config) override;

private:
    graphics_plugin m_plugin;
};
} // namespace ash::graphics