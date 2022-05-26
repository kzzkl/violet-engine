#include "ui/controls/image.hpp"

namespace ash::ui
{
image::image(graphics::resource* texture)
{
    m_type = ELEMENT_CONTROL_TYPE_IMAGE;
    m_mesh.texture = texture;
}

void image::texture(graphics::resource* texture)
{
    m_mesh.texture = texture;
    m_dirty = true;
}
} // namespace ash::ui