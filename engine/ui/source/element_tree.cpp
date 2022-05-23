#include "element_tree.hpp"
#include "context.hpp"

namespace ash::ui
{
element_tree::element_tree()
{
}

void element_tree::text(std::string_view text, const font& font, const element_rect& rect)
{
    element element = {};
    element.index_start = m_indices.size();
    element.index_end = m_indices.size() + text.size() * 6;
    element.vertex_base = m_vertex_position.size();
    element.texture = font.texture();
    m_elements.push_back(element);

    std::uint32_t pen_x = rect.x;
    std::uint32_t pen_y = rect.y + rect.height;

    std::uint32_t vertex_base = 0;
    for (char c : text)
    {
        auto& glyph = font.glyph(c);

        float x = static_cast<float>(pen_x) + static_cast<float>(glyph.bearing_x);
        float y = static_cast<float>(pen_y) - static_cast<float>(glyph.bearing_y);

        m_vertex_position.push_back(math::float2{x, y});
        m_vertex_position.push_back(
            math::float2{static_cast<float>(x + glyph.width), static_cast<float>(y)});
        m_vertex_position.push_back(math::float2{
            static_cast<float>(x + glyph.width),
            static_cast<float>(y + glyph.height)});
        m_vertex_position.push_back(math::float2{x, static_cast<float>(y + glyph.height)});

        pen_x += glyph.advance;

        m_vertex_uv.push_back(glyph.uv1);
        m_vertex_uv.push_back({glyph.uv2[0], glyph.uv1[1]});
        m_vertex_uv.push_back(glyph.uv2);
        m_vertex_uv.push_back({glyph.uv1[0], glyph.uv2[1]});

        m_indices.push_back(vertex_base);
        m_indices.push_back(vertex_base + 1);
        m_indices.push_back(vertex_base + 2);
        m_indices.push_back(vertex_base);
        m_indices.push_back(vertex_base + 2);
        m_indices.push_back(vertex_base + 3);
        vertex_base += 4;
    }
}

void element_tree::texture(graphics::resource* texture, const element_rect& rect)
{
    add_rect(rect, texture);
}

void element_tree::add_rect(const element_rect& rect, graphics::resource* texture)
{
    element element = {};
    element.index_start = m_indices.size();
    element.index_end = m_indices.size() + 6;
    element.vertex_base = m_vertex_position.size();
    element.texture = texture;
    m_elements.push_back(element);

    m_vertex_position.push_back(
        math::float2{static_cast<float>(rect.x), static_cast<float>(rect.y)});
    m_vertex_position.push_back(
        math::float2{static_cast<float>(rect.x + rect.width), static_cast<float>(rect.y)});
    m_vertex_position.push_back(math::float2{
        static_cast<float>(rect.x + rect.width),
        static_cast<float>(rect.y + rect.height)});
    m_vertex_position.push_back(
        math::float2{static_cast<float>(rect.x), static_cast<float>(rect.y + rect.height)});

    m_vertex_uv.push_back({0.0f, 0.0f});
    m_vertex_uv.push_back({1.0f, 0.0f});
    m_vertex_uv.push_back({1.0f, 1.0f});
    m_vertex_uv.push_back({0.0f, 1.0f});

    m_indices.push_back(0);
    m_indices.push_back(1);
    m_indices.push_back(2);
    m_indices.push_back(0);
    m_indices.push_back(2);
    m_indices.push_back(3);
}
} // namespace ash::ui