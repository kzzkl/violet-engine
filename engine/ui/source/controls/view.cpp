#include "ui/controls/view.hpp"

namespace violet::ui
{
view::view()
{
    m_mesh = {};
    m_mesh.scissor = true;
}
} // namespace violet::ui