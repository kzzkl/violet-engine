#pragma once

#include "ecs/entity.hpp"
#include "font.hpp"
#include "graphics_interface.hpp"
#include "layout.hpp"
#include "math/math.hpp"
#include <vector>

namespace ash::ui
{
struct element
{
    std::size_t index_start;
    std::size_t index_end;
    std::size_t vertex_base;

    graphics::resource* texture;
};

struct element_rect
{
    std::uint32_t x;
    std::uint32_t y;
    std::uint32_t width;
    std::uint32_t height;
};

class element_tree
{
public:
    element_tree();

    void text(std::string_view text, const font& font, const element_rect& rect);
    void texture(graphics::resource* texture, const element_rect& rect);

    void clear()
    {
        m_vertex_position.clear();
        m_vertex_uv.clear();
        m_indices.clear();

        m_elements.clear();
    }

    const std::vector<math::float2>& vertex_position() const noexcept { return m_vertex_position; }
    const std::vector<math::float2>& vertex_uv() const noexcept { return m_vertex_uv; }
    const std::vector<std::uint32_t>& indices() const noexcept { return m_indices; }
    const std::vector<element>& elements() const noexcept { return m_elements; }

private:
    void add_rect(const element_rect& rect, graphics::resource* texture = nullptr);

    std::vector<math::float2> m_vertex_position;
    std::vector<math::float2> m_vertex_uv;
    std::vector<std::uint32_t> m_indices;

    std::vector<element> m_elements;

    std::unique_ptr<layout> m_layout;
};
} // namespace ash::ui