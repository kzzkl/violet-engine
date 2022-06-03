#include "ui/controls/label.hpp"
#include "assert.hpp"
#include "log.hpp"

namespace ash::ui
{
label::label() : m_original_x(0.0f), m_original_y(0.0f)
{
}

label::label(std::string_view t, const font& font, std::uint32_t color)
    : m_original_x(0.0f),
      m_original_y(0.0f)
{
    reset(t, font, color);
}

void label::reset(std::string_view text, const font& font, std::uint32_t color)
{
    ASH_ASSERT(!text.empty());

    m_mesh.reset();
    m_mesh.texture = font.texture();

    float pen_x = m_original_x;
    float pen_y = m_original_y + font.heigth() * 0.73f;

    std::uint32_t vertex_base = 0;
    for (char c : text)
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

        pen_x += glyph.advance;

        m_mesh.vertex_uv.insert(
            m_mesh.vertex_uv.end(),
            {
                glyph.uv1,
                {glyph.uv2[0], glyph.uv1[1]},
                glyph.uv2,
                {glyph.uv1[0], glyph.uv2[1]}
        });
        m_mesh.vertex_color.insert(m_mesh.vertex_color.end(), 4, color);

        m_mesh.indices.insert(
            m_mesh.indices.end(),
            {vertex_base,
             vertex_base + 1,
             vertex_base + 2,
             vertex_base,
             vertex_base + 2,
             vertex_base + 3});
        vertex_base += 4;
    }

    resize(pen_x, font.heigth());
    mark_dirty();
}

void label::render(renderer& renderer)
{
    renderer.draw(RENDER_TYPE_TEXT, m_mesh);
    element::render(renderer);
}

void label::on_extent_change()
{
    auto& e = extent();

    float offset_x = e.x - m_original_x;
    float offset_y = e.y - m_original_y;
    m_original_x = e.x;
    m_original_y = e.y;

    float z = depth();
    for (auto& position : m_mesh.vertex_position)
    {
        position[0] += offset_x;
        position[1] += offset_y;
        position[2] = z;
    }
}
} // namespace ash::ui