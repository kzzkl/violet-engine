#include "control/control_system.hpp"
#include "core/engine.hpp"
#include "window/window_system.hpp"

namespace violet
{
control_system::control_system()
    : engine_system("control"),
      m_mouse_position{},
      m_mouse_position_delta{}
{
}

bool control_system::initialize(const dictionary& config)
{
    engine::on_tick().then(
        [this](float delta)
        {
            tick(delta);
        });

    engine::get_world().register_component<orbit_control>();

    return true;
}

void control_system::shutdown()
{
}

void control_system::tick(float delta)
{
    m_delta = delta;

    auto& window = engine::get_system<window_system>();

    bool mouse_hold = false;
    if (window.get_mouse().key(MOUSE_KEY_LEFT).press())
    {
        m_mouse_position_delta[0] = 0.0f;
        m_mouse_position_delta[1] = 0.0f;
        m_mouse_position[0] = window.get_mouse().get_x();
        m_mouse_position[1] = window.get_mouse().get_y();
    }
    else if (window.get_mouse().key(MOUSE_KEY_LEFT).hold())
    {
        m_mouse_position_delta[0] = window.get_mouse().get_x() - m_mouse_position[0];
        m_mouse_position_delta[1] = window.get_mouse().get_y() - m_mouse_position[1];
        m_mouse_position[0] = window.get_mouse().get_x();
        m_mouse_position[1] = window.get_mouse().get_y();

        mouse_hold = true;
    }

    view<orbit_control, transform> view(engine::get_world());
    view.each(
        [this, mouse_hold](orbit_control& orbit_control, transform& transform)
        {
            if (orbit_control.dirty || mouse_hold)
            {
                update_orbit_control(orbit_control, transform);
                orbit_control.dirty = false;
            }
        });
}

void control_system::update_orbit_control(orbit_control& orbit_control, transform& transform)
{
    static const float EPS = 0.00001f;

    orbit_control.theta += -m_mouse_position_delta[1] * m_delta * orbit_control.speed;
    orbit_control.theta = std::clamp(orbit_control.theta, EPS, PI - EPS);
    orbit_control.phi += m_mouse_position_delta[0] * m_delta * orbit_control.speed;

    auto [theta_sin, theta_cos] = sin_cos(orbit_control.theta);
    auto [phi_sin, phi_cos] = sin_cos(orbit_control.phi);

    float3 position = {
        orbit_control.r * theta_sin * phi_cos,
        orbit_control.r * theta_cos,
        -orbit_control.r * theta_sin * phi_sin};

    transform.set_position(position);
    transform.lookat(orbit_control.target, float3{0.0f, 1.0f, 0.0f});
}
} // namespace violet