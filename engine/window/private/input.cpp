#include "window/input.hpp"
#include "common/log.hpp"
#include "window_impl.hpp"

namespace violet::window
{
mouse::mouse(window_impl* impl) noexcept : m_x(0), m_y(0), m_impl(impl)
{
}

void mouse::mode(mouse_mode mode)
{
    m_x = m_y = 0;
    m_impl->mouse_mode(mode);
}

void mouse::cursor(mouse_cursor cursor)
{
    m_impl->mouse_cursor(cursor);
}

mouse_mode mouse::mode() const noexcept
{
    return m_impl->mouse_mode();
}

void mouse::tick()
{
    device_type::tick();
    if (m_impl->mouse_mode() == MOUSE_MODE_RELATIVE)
        m_x = m_y = 0;

    m_whell = 0;
}

keyboard::keyboard() noexcept
{
}
} // namespace violet::window