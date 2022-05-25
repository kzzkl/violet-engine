#include "ui/controls/plane.hpp"

namespace ash::ui
{
plane::plane(std::uint32_t color)
{
    m_type = ELEMENT_CONTROL_TYPE_BLOCK;

    m_mesh.vertex_position = {
        {0.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 0.0f}
    };
    m_mesh.vertex_uv = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };
    m_mesh.vertex_color = {color, color, color, color};
    m_mesh.indices = {0, 1, 2, 0, 2, 3};
}

void plane::extent(const element_extent& extent)
{
    m_mesh.vertex_position = {
        {extent.x,                extent.y                },
        {extent.x + extent.width, extent.y                },
        {extent.x + extent.width, extent.y + extent.height},
        {extent.x,                extent.y + extent.height}
    };
}
} // namespace ash::ui