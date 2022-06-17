#include "ui/controls/view.hpp"

namespace ash::ui
{
view::view()
{
    m_mesh = {};
    m_mesh.scissor = true;
}
} // namespace ash::ui