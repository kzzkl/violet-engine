#pragma once

#include "components/orbit_control.hpp"
#include "components/transform.hpp"
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
    void tick(float delta);
    void update_orbit_control(
        orbit_control& orbit_control,
        transform& transform,
        const int2& mouse_delta,
        int mouse_whell);

    float m_delta;

    int2 m_mouse_position;
    int2 m_mouse_position_delta;
};
} // namespace violet