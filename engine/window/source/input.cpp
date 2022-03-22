#include "input.hpp"
#include "log.hpp"

namespace ash::window
{
mouse::mouse() noexcept : m_mode(mouse_mode::CURSOR_ABSOLUTE), m_x(0), m_y(0)
{
}

void mouse::mode(mouse_mode mode)
{
    if (m_mode != mode)
    {
        m_mode = mode;
        change_mode(m_mode);
    }
}

keyboard::keyboard() noexcept
{
}
} // namespace ash::window