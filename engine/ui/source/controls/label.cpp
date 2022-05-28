#include "ui/controls/label.hpp"
#include "assert.hpp"

namespace ash::ui
{
label::label(std::string_view t, const font& font, std::uint32_t color)
    : m_original_x(0),
      m_original_y(0)
{
    text(t, font, color);
}

void label::text(std::string_view text, const font& font, std::uint32_t color)
{
    ASH_ASSERT(!text.empty());

    m_mesh.reset();
    m_baseline_offset = font.size() * 0.34;
    m_mesh.texture = font.texture();

    std::uint32_t pen_x = m_original_x;
    std::uint32_t pen_y = m_original_y;

    std::uint32_t vertex_base = 0;
    for (char c : text)
    {
        auto& glyph = font.glyph(c);

        float x = static_cast<float>(pen_x) + static_cast<float>(glyph.bearing_x);
        float y = static_cast<float>(pen_y) - static_cast<float>(glyph.bearing_y);

        m_mesh.vertex_position.push_back(math::float3{x, y, 0.0f});
        m_mesh.vertex_position.push_back(
            math::float3{static_cast<float>(x + glyph.width), static_cast<float>(y), 0.0f});
        m_mesh.vertex_position.push_back(math::float3{
            static_cast<float>(x + glyph.width),
            static_cast<float>(y + glyph.height),
            0.0f});
        m_mesh.vertex_position.push_back(
            math::float3{x, static_cast<float>(y + glyph.height), 0.0f});

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

    mark_dirty();
}

void label::render(renderer& renderer)
{
    renderer.draw(RENDER_TYPE_TEXT, m_mesh);
    element::render(renderer);
}

void label::on_extent_change(const element_extent& extent)
{
    float baseline = extent.y + extent.height * 0.5f + m_baseline_offset;

    float offset_x = extent.x - m_original_x;
    float offset_y = baseline - m_original_y;
    m_original_x = extent.x;
    m_original_y = baseline;

    float z = depth();
    for (auto& position : m_mesh.vertex_position)
    {
        position[0] += offset_x;
        position[1] += offset_y;
        position[2] = z;
    }
}
} // namespace ash::ui