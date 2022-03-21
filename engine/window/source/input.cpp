#include "input.hpp"
#include "log.hpp"

namespace ash::window
{
mouse::mouse() : m_mode(mouse_mode::CURSOR_ABSOLUTE), m_x(0), m_y(0)
{
}

void mouse::reset_relative_cursor()
{
    if (m_mode == mouse_mode::CURSOR_RELATIVE)
        m_x = m_y = 0;
}

void mouse::mode(mouse_mode mode)
{
    if (m_mode != mode)
    {
        if (mode == mouse_mode::CURSOR_ABSOLUTE)
            clip_cursor(false);
        else if (mode == mouse_mode::CURSOR_RELATIVE)
            clip_cursor(true);
        show_cursor(mode == mouse_mode::CURSOR_ABSOLUTE);
        m_mode = mode;
        m_x = m_y = 0;
    }
}

keyboard::keyboard()
{
}
} // namespace ash::window