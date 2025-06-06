#include "control/control_system.hpp"
#include "scene/scene_system.hpp"
#include "window/window_system.hpp"

namespace violet
{
control_system::control_system()
    : system("control"),
      m_mouse_position{},
      m_mouse_position_delta{}
{
}

void control_system::install(application& app)
{
    app.install<scene_system>();
    app.install<window_system>();
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

void control_system::tick()
{
    auto& window = get_system<window_system>();

    bool mouse_hold = false;
    int mouse_wheel = window.get_mouse().get_wheel();

    if (window.get_mouse().key(MOUSE_KEY_LEFT).hold())
    {
        m_mouse_position_delta.x = window.get_mouse().get_x() - m_mouse_position[0];
        m_mouse_position_delta.y = window.get_mouse().get_y() - m_mouse_position[1];

        mouse_hold = true;
    }
    else
    {
        m_mouse_position_delta.x = 0;
        m_mouse_position_delta.y = 0;
    }

    m_mouse_position.x = window.get_mouse().get_x();
    m_mouse_position.y = window.get_mouse().get_y();

    if (mouse_hold || mouse_wheel != 0)
    {
        get_world().get_view().write<orbit_control_component>().write<transform_component>().each(
            [this,
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
    orbit_control.radius += static_cast<float>(-mouse_wheel) * orbit_control.radius_speed;
    orbit_control.radius = std::max(orbit_control.min_radius, orbit_control.radius);

    auto [theta_sin, theta_cos] = math::sin_cos(orbit_control.theta);
    auto [phi_sin, phi_cos] = math::sin_cos(orbit_control.phi);

    vec3f p = {
        orbit_control.radius * theta_sin * phi_cos,
        orbit_control.radius * theta_cos,
        -orbit_control.radius * theta_sin * phi_sin};
    transform.set_position(vector::add(p, orbit_control.target));
    transform.lookat(orbit_control.target, vec3f{0.0f, 1.0f, 0.0f});
}
} // namespace violet