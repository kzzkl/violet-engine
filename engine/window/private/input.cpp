#include "window/input.hpp"
#include "common/log.hpp"
#include "window_impl.hpp"

namespace violet
{
mouse::mouse(window_impl* impl) noexcept : m_x(0), m_y(0), m_impl(impl)
{
}

void mouse::set_mode(mouse_mode mode)
{
    m_x = m_y = 0;
    m_impl->set_mouse_mode(mode);
}

mouse_mode mouse::get_mode() const noexcept
{
    return m_impl->get_mouse_mode();
}

void mouse::set_cursor(mouse_cursor cursor)
{
    m_impl->set_mouse_cursor(cursor);
}

void mouse::tick()
{
    device_type::tick();
    if (m_impl->get_mouse_mode() == MOUSE_MODE_RELATIVE)
        m_x = m_y = 0;

    m_whell = 0;
}

keyboard::keyboard() noexcept
{
}
} // namespace violet