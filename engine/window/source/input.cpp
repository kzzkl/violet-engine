#include "window/input.hpp"
#include "log.hpp"
#include "window/window_impl.hpp"

namespace ash::window
{
mouse::mouse(window_impl* impl) noexcept : m_mode(MOUSE_MODE_ABSOLUTE), m_x(0), m_y(0), m_impl(impl)
{
}

void mouse::mode(mouse_mode mode)
{
    if (m_mode != mode)
    {
        m_x = m_y = 0;
        m_mode = mode;
        m_impl->change_mouse_mode(mode);
    }
}

void mouse::tick()
{
    device_type::tick();
    if (m_mode == MOUSE_MODE_RELATIVE)
        m_x = m_y = 0;

    m_whell = 0;
}

keyboard::keyboard() noexcept
{
}
} // namespace ash::window