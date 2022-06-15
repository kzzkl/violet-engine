#include "ui/controls/font_icon.hpp"
#include "ui/font.hpp"

namespace ash::ui
{
font_icon::font_icon(std::uint32_t index, const font_icon_theme& theme)
    : m_icon_scale(theme.icon_scale)
{
    m_font = theme.icon_font;

    m_mesh.vertex_position.resize(4);
    m_mesh.vertex_color.resize(4);
    m_mesh.indices = {0, 1, 2, 0, 2, 3};
    icon(index);
    icon_color(theme.icon_color);
    width(theme.icon_font->heigth() * theme.icon_scale);
    height(theme.icon_font->heigth() * theme.icon_scale);
}

void font_icon::icon(std::uint32_t index)
{
    m_mesh.texture = m_font->texture();

    auto& glyph = m_font->glyph(index);

    float x = static_cast<float>(glyph.bearing_x);
    float y = m_font->heigth() - static_cast<float>(glyph.bearing_y);
    m_icon_width = glyph.width;
    m_icon_height = glyph.height;

    m_mesh.vertex_uv = {
        glyph.uv1,
        {glyph.uv2[0], glyph.uv1[1]},
        glyph.uv2,
        {glyph.uv1[0], glyph.uv2[1]}
    };

    on_extent_change(extent());
    mark_dirty();
}

void font_icon::icon_scale(float scale)
{
    m_icon_scale = scale;
    on_extent_change(extent());
    mark_dirty();
}

void font_icon::icon_color(std::uint32_t color)
{
    for (auto& c : m_mesh.vertex_color)
        c = color;

    mark_dirty();
}

void font_icon::render(renderer& renderer)
{
    renderer.draw(RENDER_TYPE_TEXT, m_mesh);
    element::render(renderer);
}

void font_icon::on_extent_change(const element_extent& extent)
{
    float width = m_icon_width * m_icon_scale;
    float height = m_icon_height * m_icon_scale;

    float x = (extent.width - width) * 0.5f + extent.x;
    float y = (extent.height - height) * 0.5f + extent.y;
    float z = depth();

    m_mesh.vertex_position[0] = {x, y, z};
    m_mesh.vertex_position[1] = {x + width, y, z};
    m_mesh.vertex_position[2] = {x + width, y + height, z};
    m_mesh.vertex_position[3] = {x, y + height, z};
}
} // namespace ash::ui