#pragma once

#include "graphics_interface.hpp"

namespace ash::graphics::d3d12
{
class d3d12_renderer : public ash::graphics::external::renderer
{
public:
    virtual bool initialize(ash::graphics::external::window_handle handle) override;

    virtual void begin_frame() override;
    virtual void end_frame() override;

private:
};
} // namespace ash::graphics::d3d12