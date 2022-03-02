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

void mouse::set_mode(mouse_mode mode)
{
    if (m_mode != mode)
    {
        if (mode == mouse_mode::CURSOR_ABSOLUTE)
            set_cursor_clip(false);
        else if (mode == mouse_mode::CURSOR_RELATIVE)
            set_cursor_clip(true);
        show_cursor(mode == mouse_mode::CURSOR_ABSOLUTE);
        m_mode = mode;
        m_x = m_y = 0;
    }
}

void mouse::set_cursor_clip(bool clip)
{
}

keyboard::keyboard()
{
}
} // namespace ash::window