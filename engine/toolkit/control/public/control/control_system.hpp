#pragma once

#include "components/orbit_control_component.hpp"
#include "components/transform_component.hpp"
#include "core/engine_system.hpp"

namespace violet
{
class control_system : public engine_system
{
public:
    control_system();

    virtual bool initialize(const dictionary& config);
    virtual void shutdown();

private:
    void tick();
    void update_orbit_control(
        orbit_control_component& orbit_control,
        transform_component& transform,
        int mouse_wheel);

    vec2i m_mouse_position;
    vec2i m_mouse_position_delta;
};
} // namespace violet