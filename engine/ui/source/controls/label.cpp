#include "ui/controls/label.hpp"
#include "assert.hpp"

namespace ash::ui
{
label::label() : m_original_x(0.0f), m_original_y(0.0f)
{
}

label::label(std::string_view content, const label_style& style)
    : m_original_x(0.0f),
      m_original_y(0.0f)
{
    text(content, *style.text_font);
    text_color(style.text_color);
}

void label::text(std::string_view content, const font& font)
{
    ASH_ASSERT(!content.empty());

    m_mesh.vertex_position.clear();
    m_mesh.vertex_uv.clear();
    m_mesh.indices.clear();
    m_mesh.texture = font.texture();

    float pen_x = m_original_x;
    float pen_y = m_original_y + font.heigth() * 0.73f;

    std::uint32_t vertex_base = 0;
    for (char c : content)
    {
        auto& glyph = font.glyph(c);

        float x = pen_x + glyph.bearing_x;
        float y = pen_y - glyph.bearing_y;

        m_mesh.vertex_position.insert(
            m_mesh.vertex_position.end(),
            {
                {x,               y,                0.0f},
                {x + glyph.width, y,                0.0f},
                {x + glyph.width, y + glyph.height, 0.0f},
                {x,               y + glyph.height, 0.0f}
        });

        m_mesh.vertex_uv.insert(
            m_mesh.vertex_uv.end(),
            {
                glyph.uv1,
                {glyph.uv2[0], glyph.uv1[1]},
                glyph.uv2,
                {glyph.uv1[0], glyph.uv2[1]}
        });
        m_mesh.indices.insert(
            m_mesh.indices.end(),
            {vertex_base,
             vertex_base + 1,
             vertex_base + 2,
             vertex_base,
             vertex_base + 2,
             vertex_base + 3});

        vertex_base += 4;
        pen_x += glyph.advance;
    }

    if (m_mesh.vertex_color.empty())
        m_mesh.vertex_color.resize(m_mesh.vertex_position.size());
    else
        m_mesh.vertex_color.resize(m_mesh.vertex_position.size(), m_mesh.vertex_color[0]);

    width(pen_x - m_original_x);
    height(font.heigth());

    m_text = content;

    mark_dirty();
}

void label::text_color(std::uint32_t color)
{
    for (auto& c : m_mesh.vertex_color)
        c = color;

    mark_dirty();
}

void label::render(renderer& renderer)
{
    renderer.draw(RENDER_TYPE_TEXT, m_mesh);
    element::render(renderer);
}

void label::on_extent_change(const element_extent& extent)
{
    float offset_x = extent.x - m_original_x;
    float offset_y = extent.y - m_original_y;
    m_original_x = extent.x;
    m_original_y = extent.y;

    float z = depth();
    for (auto& position : m_mesh.vertex_position)
    {
        position[0] += offset_x;
        position[1] += offset_y;
        position[2] = z;
    }
}
} // namespace ash::ui