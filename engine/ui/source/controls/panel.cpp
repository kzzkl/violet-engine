#include "ui/controls/panel.hpp"

namespace violet::ui
{
panel::panel(std::uint32_t color, bool scissor)
{
    m_position = {
        math::float2{0.0f, 0.0f},
        math::float2{0.0f, 0.0f},
        math::float2{0.0f, 0.0f},
        math::float2{0.0f, 0.0f}
    };
    m_uv = {
        math::float2{0.0f, 0.0f},
        math::float2{1.0f, 0.0f},
        math::float2{1.0f, 1.0f},
        math::float2{0.0f, 1.0f}
    };
    m_color = {color, color, color, color};
    m_indices = {0, 1, 2, 0, 2, 3};

    m_mesh = {
        .type = ELEMENT_MESH_TYPE_BLOCK,
        .position = m_position.data(),
        .uv = m_uv.data(),
        .color = m_color.data(),
        .vertex_count = 4,
        .indices = m_indices.data(),
        .index_count = 6,
        .scissor = scissor,
        .texture = nullptr};
}

void panel::color(std::uint32_t color) noexcept
{
    if (m_color[0] != color)
    {
        m_color = {color, color, color, color};
        mark_dirty();
    }
}

void panel::on_extent_change(float width, float height)
{
    m_position[1] = {width, 0.0f};
    m_position[2] = {width, height};
    m_position[3] = {0.0f, height};
}
} // namespace violet::ui