#include "ui/controls/font_icon.hpp"

namespace ash::ui
{
font_icon::font_icon(std::uint32_t index, const font& font, float scale, std::uint32_t color)
    : m_icon_scale(scale),
      m_icon_color(color)
{
    icon(index, font);
    width(font.heigth());
    height(font.heigth());
}

void font_icon::icon(std::uint32_t index, const font& font)
{
    m_mesh.reset();
    m_mesh.texture = font.texture();

    auto& glyph = font.glyph(index);

    float x = static_cast<float>(glyph.bearing_x);
    float y = font.heigth() - static_cast<float>(glyph.bearing_y);
    m_icon_width = glyph.width;
    m_icon_height = glyph.height;

    m_mesh.vertex_position.resize(4);
    m_mesh.vertex_uv = {
        glyph.uv1,
        {glyph.uv2[0], glyph.uv1[1]},
        glyph.uv2,
        {glyph.uv1[0], glyph.uv2[1]}
    };
    m_mesh.vertex_color = {m_icon_color, m_icon_color, m_icon_color, m_icon_color};
    m_mesh.indices = {0, 1, 2, 0, 2, 3};

    on_extent_change();
    mark_dirty();
}

void font_icon::icon_scale(float scale)
{
    m_icon_scale = scale;
    on_extent_change();
    mark_dirty();
}

void font_icon::icon_color(std::uint32_t color)
{
    for (auto& c : m_mesh.vertex_color)
        c = color;
    m_icon_color = color;

    mark_dirty();
}

void font_icon::render(renderer& renderer)
{
    renderer.draw(RENDER_TYPE_TEXT, m_mesh);
    element::render(renderer);
}

void font_icon::on_extent_change()
{
    float width = m_icon_width * m_icon_scale;
    float height = m_icon_height * m_icon_scale;

    auto& e = extent();
    float x = (e.width - width) * 0.5f + e.x;
    float y = (e.height - height) * 0.5f + e.y;
    float z = depth();

    m_mesh.vertex_position[0] = {x, y, z};
    m_mesh.vertex_position[1] = {x + width, y, z};
    m_mesh.vertex_position[2] = {x + width, y + height, z};
    m_mesh.vertex_position[3] = {x, y + height, z};
}
} // namespace ash::ui