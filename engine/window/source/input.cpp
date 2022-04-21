#include "input.hpp"
#include "log.hpp"
#include "window_impl.hpp"

namespace ash::window
{
mouse::mouse(window_impl* impl) noexcept
    : m_mode(mouse_mode::CURSOR_ABSOLUTE),
      m_x(0),
      m_y(0),
      m_impl(impl)
{
}

void mouse::mode(mouse_mode mode)
{
    if (m_mode != mode)
    {
        m_mode = mode;
        m_impl->change_mouse_mode(mode);
    }
}

keyboard::keyboard() noexcept
{
}
} // namespace ash::window