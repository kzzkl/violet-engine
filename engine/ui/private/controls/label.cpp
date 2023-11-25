#include "ui/controls/label.hpp"
#include "ui/font.hpp"
#include <cassert>

namespace violet::ui
{
label::label()
{
}

label::label(std::string_view content, const label_theme& theme)
{
    m_font = theme.text_font;

    text(content);
    text_color(theme.text_color);

    m_mesh.type = ELEMENT_MESH_TYPE_TEXT;
    m_mesh.scissor = false;
}

void label::text(std::string_view content)
{
    assert(!content.empty());

    m_position.clear();
    m_uv.clear();
    m_indices.clear();

    float pen_x = 0.0f;
    float pen_y = m_font->heigth() * 0.75f;

    std::uint32_t vertex_base = 0;
    for (char c : content)
    {
        auto& glyph = m_font->glyph(c);

        float x = pen_x + glyph.bearing_x;
        float y = pen_y - glyph.bearing_y;

        m_position.insert(
            m_position.end(),
            {
                {x,               y               },
                {x + glyph.width, y               },
                {x + glyph.width, y + glyph.height},
                {x,               y + glyph.height}
        });

        m_uv.insert(
            m_uv.end(),
            {
                glyph.uv1,
                {glyph.uv2[0], glyph.uv1[1]},
                glyph.uv2,
                {glyph.uv1[0], glyph.uv2[1]}
        });
        m_indices.insert(
            m_indices.end(),
            {vertex_base,
             vertex_base + 1,
             vertex_base + 2,
             vertex_base,
             vertex_base + 2,
             vertex_base + 3});

        vertex_base += 4;
        pen_x += glyph.advance;
    }

    if (m_color.empty())
        m_color.resize(m_position.size());
    else
        m_color.resize(m_position.size(), m_color[0]);

    m_mesh.position = m_position.data();
    m_mesh.uv = m_uv.data();
    m_mesh.color = m_color.data();
    m_mesh.vertex_count = m_position.size();
    m_mesh.indices = m_indices.data();
    m_mesh.index_count = m_indices.size();
    m_mesh.texture = m_font->texture();

    layout()->set_width(pen_x);
    layout()->set_height(m_font->heigth());

    m_text = content;

    mark_dirty();
}

void label::text_color(std::uint32_t color)
{
    for (auto& c : m_color)
        c = color;

    mark_dirty();
}
} // namespace violet::ui