#include "window/input.hpp"
#include "window_impl.hpp"

namespace violet
{
mouse::mouse(window_impl* impl) noexcept
    : m_impl(impl)
{
}

void mouse::set_mode(mouse_mode mode)
{
    m_impl->set_mouse_mode(mode);
}

mouse_mode mouse::get_mode() const noexcept
{
    return m_impl->get_mouse_mode();
}

void mouse::set_cursor(mouse_cursor cursor)
{
    m_impl->set_cursor(cursor);
}

void mouse::set_screen_position(const vec2i& position)
{
    m_impl->set_cursor_position(position);
}

vec2i mouse::get_screen_position() const noexcept
{
    return m_impl->get_screen_position(m_position);
}

void mouse::tick()
{
    device_type::tick();
    m_wheel = 0;
}

keyboard::keyboard() noexcept = default;
} // namespace violet