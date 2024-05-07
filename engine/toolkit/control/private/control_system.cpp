#include "control/control_system.hpp"
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
    on_tick().then(
        [this](float delta)
        {
            tick(delta);
        });

    get_world().register_component<orbit_control>();

    return true;
}

void control_system::shutdown()
{
}

void control_system::tick(float delta)
{
    auto& window = get_system<window_system>();

    bool mouse_hold = false;
    int mouse_whell = window.get_mouse().get_whell();
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

    view<orbit_control, transform> view(get_world());
    view.each(
        [this, mouse_hold, mouse_whell](orbit_control& orbit_control, transform& transform)
        {
            if (orbit_control.dirty || mouse_hold || mouse_whell != 0)
            {
                update_orbit_control(orbit_control, transform, mouse_whell);
                orbit_control.dirty = false;
            }
        });
}

void control_system::update_orbit_control(
    orbit_control& orbit_control,
    transform& transform,
    int mouse_whell)
{
    static const float EPS = 0.00001f;

    auto window_rect = get_system<window_system>().get_extent();

    orbit_control.theta +=
        -m_mouse_position_delta[1] * orbit_control.theta_speed / window_rect.height;
    orbit_control.theta = std::clamp(orbit_control.theta, EPS, PI - EPS);
    orbit_control.phi += m_mouse_position_delta[0] * orbit_control.phi_speed / window_rect.width;
    orbit_control.r += -mouse_whell * orbit_control.r_speed;
    orbit_control.r = std::max(1.0f, orbit_control.r);

    auto [theta_sin, theta_cos] = sin_cos(orbit_control.theta);
    auto [phi_sin, phi_cos] = sin_cos(orbit_control.phi);

    float3 position = {
        orbit_control.r * theta_sin * phi_cos,
        orbit_control.r * theta_cos,
        -orbit_control.r * theta_sin * phi_sin};
    position = vector::add(position, orbit_control.target);

    transform.set_position(position);
    transform.lookat(orbit_control.target, float3{0.0f, 1.0f, 0.0f});
}
} // namespace violet