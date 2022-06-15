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

void panel::on_extent_change(const element_extent& extent)
{
    float z = depth();
    m_mesh.vertex_position = {
        {extent.x,                extent.y,                 z},
        {extent.x + extent.width, extent.y,                 z},
        {extent.x + extent.width, extent.y + extent.height, z},
        {extent.x,                extent.y + extent.height, z}
    };
}

view_panel::view_panel(std::uint32_t color) : panel(color)
{
}

void view_panel::render(renderer& renderer)
{
    renderer.scissor_push(layout_extent());
    panel::render(renderer);
    renderer.scissor_pop();
}
} // namespace ash::ui