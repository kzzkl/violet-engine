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
    task_graph& task_graph = get_task_graph();
    task_group& update = task_graph.get_group("Update");

    task_graph.add_task()
        .set_name("Update Control")
        .set_group(update)
        .set_execute(
            [this]()
            {
                tick();
            });

    get_world().register_component<orbit_control_component>();

    return true;
}

void control_system::shutdown() {}

void control_system::tick()
{
    auto& window = get_system<window_system>();

    bool mouse_hold = false;
    int mouse_wheel = window.get_mouse().get_wheel();
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

    if (mouse_hold || mouse_wheel != 0)
    {
        get_world().get_view().write<orbit_control_component>().write<transform_component>().each(
            [this,
             mouse_hold,
             mouse_wheel](orbit_control_component& orbit_control, transform_component& transform)
            {
                update_orbit_control(orbit_control, transform, mouse_wheel);
            });
    }
}

void control_system::update_orbit_control(
    orbit_control_component& orbit_control,
    transform_component& transform,
    int mouse_wheel)
{
    static const float eps = 0.00001f;

    auto window_rect = get_system<window_system>().get_extent();

    orbit_control.theta += orbit_control.theta_speed *
                           static_cast<float>(-m_mouse_position_delta[1]) /
                           static_cast<float>(window_rect.height);
    orbit_control.theta = std::clamp(orbit_control.theta, eps, math::PI - eps);
    orbit_control.phi += orbit_control.phi_speed * static_cast<float>(m_mouse_position_delta[0]) /
                         static_cast<float>(window_rect.width);
    orbit_control.r += static_cast<float>(-mouse_wheel) * orbit_control.r_speed;
    orbit_control.r = std::max(1.0f, orbit_control.r);

    auto [theta_sin, theta_cos] = math::sin_cos(orbit_control.theta);
    auto [phi_sin, phi_cos] = math::sin_cos(orbit_control.phi);

    vec3f p = {
        orbit_control.r * theta_sin * phi_cos,
        orbit_control.r * theta_cos,
        -orbit_control.r * theta_sin * phi_sin};
    transform.position = vector::add(p, orbit_control.target);
    transform.lookat(orbit_control.target, vec3f{0.0f, 1.0f, 0.0f});
}
} // namespace violet