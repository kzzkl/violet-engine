#include "ui/controls/dock_panel.hpp"
#include "log.hpp"

namespace ash::ui
{
dock_panel::dock_panel(std::uint32_t color)
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

    /*on_mouse_drag_begin = [this](int x, int y) {
        log::debug("drag begin");
        position_type(LAYOUT_POSITION_TYPE_ABSOLUTE);
    };
    on_mouse_drag_end = [this](int x, int y) {
        log::debug("drag end");
    };

    on_mouse_drag = [this](int x, int y) {
        position(x, LAYOUT_EDGE_LEFT);
        position(y, LAYOUT_EDGE_TOP);
    };*/
}

void dock_panel::color(std::uint32_t color) noexcept
{
    if (m_mesh.vertex_color[0] != color)
    {
        m_mesh.vertex_color = {color, color, color, color};
        mark_dirty();
    }
}

void dock_panel::render(renderer& renderer)
{
    renderer.draw(RENDER_TYPE_BLOCK, m_mesh);
    element::render(renderer);
}

void dock_panel::on_extent_change()
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