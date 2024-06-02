#pragma once

#include "components/orbit_control.hpp"
#include "components/transform.hpp"
#include "core/engine_module.hpp"

namespace violet
{
class control_module : public engine_module
{
public:
    control_module();

    virtual bool initialize(const dictionary& config);
    virtual void shutdown();

private:
    void tick(float delta);
    void update_orbit_control(orbit_control& orbit_control, transform& transform, int mouse_wheel);

    int2 m_mouse_position;
    int2 m_mouse_position_delta;
};
} // namespace violet