#include "ui/controls/plane.hpp"

namespace ash::ui
{
plane::plane(std::uint32_t color)
{
    m_type = ELEMENT_CONTROL_TYPE_BLOCK;

    m_mesh.vertex_position = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
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
    float z = depth();
    m_mesh.vertex_position = {
        {extent.x,                extent.y,                 z},
        {extent.x + extent.width, extent.y,                 z},
        {extent.x + extent.width, extent.y + extent.height, z},
        {extent.x,                extent.y + extent.height, z}
    };
}
} // namespace ash::ui