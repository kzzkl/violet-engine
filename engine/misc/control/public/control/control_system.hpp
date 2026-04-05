#pragma once

#include "core/engine.hpp"
#include "math/types.hpp"

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
    void update_orbit_control();
    void update_first_person_control();

    vec2i m_mouse_position;
    vec2f m_mouse_position_delta;

    bool m_mouse_hold;
    int m_mouse_wheel;
};
} // namespace violet