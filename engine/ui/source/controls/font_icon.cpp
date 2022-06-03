#include "ui/controls/font_icon.hpp"

namespace ash::ui
{
font_icon::font_icon(std::uint32_t index, const font& font, float scale, std::uint32_t color)
{
    reset(index, font, scale, color);
}

void font_icon::reset(std::uint32_t index, const font& font, float scale, std::uint32_t color)
{
    m_mesh.reset();
    m_mesh.texture = font.texture();

    auto& glyph = font.glyph(index);

    float x = static_cast<float>(glyph.bearing_x);
    float y = font.heigth() - static_cast<float>(glyph.bearing_y);
    m_width = static_cast<float>(glyph.width) * scale;
    m_height = static_cast<float>(glyph.height) * scale;

    m_mesh.vertex_position.resize(4);
    m_mesh.vertex_uv = {
        glyph.uv1,
        {glyph.uv2[0], glyph.uv1[1]},
        glyph.uv2,
        {glyph.uv1[0], glyph.uv2[1]}
    };
    m_mesh.vertex_color = {color, color, color, color};
    m_mesh.indices = {0, 1, 2, 0, 2, 3};

    on_extent_change();
    mark_dirty();
}

void font_icon::render(renderer& renderer)
{
    renderer.draw(RENDER_TYPE_TEXT, m_mesh);
    element::render(renderer);
}

void font_icon::on_extent_change()
{
    auto& e = extent();

    float x = (e.width - m_width) * 0.5f + e.x;
    float y = (e.height - m_height) * 0.5f + e.y;
    float z = depth();

    m_mesh.vertex_position[0] = {x, y, z};
    m_mesh.vertex_position[1] = {x + m_width, y, z};
    m_mesh.vertex_position[2] = {x + m_width, y + m_height, z};
    m_mesh.vertex_position[3] = {x, y + m_height, z};
}
} // namespace ash::ui