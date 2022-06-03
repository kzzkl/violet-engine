#include "ui/controls/panel.hpp"

namespace ash::ui
{
panel::panel(std::uint32_t color)
{
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

void panel::color(std::uint32_t color) noexcept
{
    if (m_mesh.vertex_color[0] != color)
    {
        m_mesh.vertex_color = {color, color, color, color};
        mark_dirty();
    }
}

void panel::render(renderer& renderer)
{
    renderer.draw(RENDER_TYPE_BLOCK, m_mesh);
    element::render(renderer);
}

void panel::on_extent_change()
{
    auto& e = extent();
    float z = depth();
    m_mesh.vertex_position = {
        {e.x,           e.y,            z},
        {e.x + e.width, e.y,            z},
        {e.x + e.width, e.y + e.height, z},
        {e.x,           e.y + e.height, z}
    };
}
} // namespace ash::ui