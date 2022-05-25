#include "ui/controls/label.hpp"
#include "assert.hpp"

namespace ash::ui
{
label::label(std::string_view text, const font& font, std::uint32_t color)
    : m_original_x(0),
      m_original_y(0)
{
    ASH_ASSERT(!text.empty());

    m_type = ELEMENT_CONTROL_TYPE_TEXT;
    m_mesh.texture = font.texture();

    std::uint32_t pen_x = 0;
    std::uint32_t pen_y = 0;

    std::uint32_t vertex_base = 0;
    for (char c : text)
    {
        auto& glyph = font.glyph(c);

        float x = static_cast<float>(pen_x) + static_cast<float>(glyph.bearing_x);
        float y = static_cast<float>(pen_y) - static_cast<float>(glyph.bearing_y);

        m_mesh.vertex_position.push_back(math::float2{x, y});
        m_mesh.vertex_position.push_back(
            math::float2{static_cast<float>(x + glyph.width), static_cast<float>(y)});
        m_mesh.vertex_position.push_back(math::float2{
            static_cast<float>(x + glyph.width),
            static_cast<float>(y + glyph.height)});
        m_mesh.vertex_position.push_back(math::float2{x, static_cast<float>(y + glyph.height)});

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
}

void label::extent(const element_extent& extent)
{
    // baseline position = 0.7 * height
    float offset_x = extent.x - m_original_x;
    float offset_y = extent.y + extent.height * 0.7f - m_original_y;
    m_original_x = extent.x;
    m_original_y = extent.y + extent.height * 0.7f;

    for (auto& position : m_mesh.vertex_position)
    {
        position[0] += offset_x;
        position[1] += offset_y;
    }
}
} // namespace ash::ui