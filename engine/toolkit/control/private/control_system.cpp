#include "control/control_system.hpp"
#include "components/first_person_control_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/transform_component.hpp"
#include "scene/scene_system.hpp"
#include "window/window_system.hpp"

namespace violet
{
static const float EPSILON = 0.00001f;

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

    auto& world = get_world();
    world.register_component<orbit_control_component>();
    world.register_component<first_person_control_component>();

    return true;
}

void control_system::tick()
{
    auto& window = get_system<window_system>();
    auto window_size = window.get_window_size();

    m_mouse_hold = false;
    m_mouse_wheel = window.get_mouse().get_wheel();

    vec2i mouse_position_delta = {};

    if (window.get_mouse().key(MOUSE_KEY_RIGHT).press())
    {
        window.get_mouse().set_mode(MOUSE_MODE_RELATIVE);
    }
    else if (window.get_mouse().key(MOUSE_KEY_RIGHT).release())
    {
        window.get_mouse().set_mode(MOUSE_MODE_ABSOLUTE);
    }

    if (window.get_mouse().key(MOUSE_KEY_RIGHT).hold())
    {
        mouse_position_delta = window.get_mouse().get_window_position() - m_mouse_position;
        m_mouse_hold = true;
    }

    m_mouse_position = window.get_mouse().get_window_position();

    m_mouse_position_delta.x =
        static_cast<float>(mouse_position_delta[0]) / static_cast<float>(window_size.width);
    m_mouse_position_delta.y =
        static_cast<float>(mouse_position_delta[1]) / static_cast<float>(window_size.height);

    update_orbit_control();
    update_first_person_control();
}

void control_system::update_orbit_control()
{
    if (!m_mouse_hold && m_mouse_wheel == 0)
    {
        return;
    }

    auto& world = get_world();

    world.get_view().write<orbit_control_component>().write<transform_component>().each(
        [&](orbit_control_component& orbit_control, transform_component& transform)
        {
            orbit_control.theta += orbit_control.theta_speed * m_mouse_position_delta[1];
            orbit_control.theta = std::clamp(orbit_control.theta, EPSILON, math::PI - EPSILON);
            orbit_control.phi += orbit_control.phi_speed * -m_mouse_position_delta[0];
            orbit_control.radius += static_cast<float>(-m_mouse_wheel) * orbit_control.radius_speed;
            orbit_control.radius = std::max(orbit_control.min_radius, orbit_control.radius);

            auto [theta_sin, theta_cos] = math::sin_cos(orbit_control.theta);
            auto [phi_sin, phi_cos] = math::sin_cos(orbit_control.phi);

            vec3f p = {
                orbit_control.radius * theta_sin * phi_cos,
                orbit_control.radius * theta_cos,
                -orbit_control.radius * theta_sin * phi_sin};
            transform.set_position(vector::add(p, orbit_control.target));
            transform.lookat(orbit_control.target, vec3f{0.0f, 1.0f, 0.0f});
        });
}

void control_system::update_first_person_control()
{
    auto& keyboard = get_system<window_system>().get_keyboard();

    float forward_input = 0.0f;
    forward_input += keyboard.key(KEYBOARD_KEY_W).down() ? 1.0f : 0.0f;
    forward_input -= keyboard.key(KEYBOARD_KEY_S).down() ? 1.0f : 0.0f;

    float right_input = 0.0f;
    right_input += keyboard.key(KEYBOARD_KEY_D).down() ? 1.0f : 0.0f;
    right_input -= keyboard.key(KEYBOARD_KEY_A).down() ? 1.0f : 0.0f;

    if (forward_input == 0.0f && right_input == 0.0f && !m_mouse_hold)
    {
        return;
    }

    auto& world = get_world();

    float delta_time = get_timer().get_frame_delta();

    world.get_view().write<first_person_control_component>().write<transform_component>().each(
        [&](first_person_control_component& first_person_control, transform_component& transform)
        {
            float move_speed = first_person_control.move_speed * delta_time;

            auto forward = quaternion::mul_vec(
                transform.get_rotation(),
                vec3f{.x = 0.0f, .y = 0.0f, .z = 1.0f});
            auto right = quaternion::mul_vec(
                transform.get_rotation(),
                vec3f{.x = 1.0f, .y = 0.0f, .z = 0.0f});

            vec3f position = transform.get_position();
            position += move_speed * forward_input * forward;
            position += move_speed * right_input * right;
            transform.set_position(position);

            first_person_control.yaw += first_person_control.yaw_speed * m_mouse_position_delta[0];
            first_person_control.pitch +=
                first_person_control.pitch_speed * m_mouse_position_delta[1];
            first_person_control.pitch = std::clamp(
                first_person_control.pitch,
                -math::HALF_PI + EPSILON,
                math::HALF_PI - EPSILON);

            transform.set_rotation(
                quaternion::from_euler(
                    vec3f{first_person_control.pitch, first_person_control.yaw, 0.0f}));
        });
}
} // namespace violet