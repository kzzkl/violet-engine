#pragma once

#include "components/orbit_control_component.hpp"
#include "components/transform_component.hpp"
#include "core/engine.hpp"

namespace violet
{
class control_system : public system
{
public:
    control_system();

    void install(application& app) override;
    bool initialize(const dictionary& config) override;

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