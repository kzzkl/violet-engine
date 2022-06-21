#include "ui/controls/font_icon.hpp"
#include "ui/font.hpp"

namespace ash::ui
{
font_icon::font_icon(std::uint32_t index, const font_icon_theme& theme)
    : m_icon_scale(theme.icon_scale)
{
    m_font = theme.icon_font;

    m_indices = {0, 1, 2, 0, 2, 3};

    icon(index);
    icon_color(theme.icon_color);
    width(theme.icon_font->heigth() * theme.icon_scale);
    height(theme.icon_font->heigth() * theme.icon_scale);

    m_mesh = {
        .type = ELEMENT_MESH_TYPE_TEXT,
        .position = m_position.data(),
        .uv = m_uv.data(),
        .color = m_color.data(),
        .vertex_count = 4,
        .indices = m_indices.data(),
        .index_count = 6,
        .scissor = false,
        .texture = m_font->texture()};
}

void font_icon::icon(std::uint32_t index)
{
    m_mesh.texture = m_font->texture();

    auto& glyph = m_font->glyph(index);

    float x = static_cast<float>(glyph.bearing_x);
    float y = m_font->heigth() - static_cast<float>(glyph.bearing_y);
    m_icon_width = glyph.width;
    m_icon_height = glyph.height;

    m_uv = {
        glyph.uv1,
        {glyph.uv2[0], glyph.uv1[1]},
        glyph.uv2,
        {glyph.uv1[0], glyph.uv2[1]}
    };

    auto& e = extent();
    on_extent_change(e.width, e.height);
    mark_dirty();
}

void font_icon::icon_scale(float scale)
{
    m_icon_scale = scale;
    auto& e = extent();
    on_extent_change(e.width, e.height);
    mark_dirty();
}

void font_icon::icon_color(std::uint32_t color)
{
    for (auto& c : m_color)
        c = color;

    mark_dirty();
}

void font_icon::on_extent_change(float width, float height)
{
    float icon_width = m_icon_width * m_icon_scale;
    float icon_height = m_icon_height * m_icon_scale;

    float x = (width - icon_width) * 0.5f;
    float y = (height - icon_height) * 0.5f;

    m_position[0] = {x, y};
    m_position[1] = {x + icon_width, y};
    m_position[2] = {x + icon_width, y + icon_height};
    m_position[3] = {x, y + icon_height};
}
} // namespace ash::ui