#pragma once

#include "graphics_interface.hpp"
#include <array>
#include <memory>
#include <string_view>

namespace ash::ui
{
struct glyph_data
{
    std::uint32_t width;
    std::uint32_t height;
    std::int32_t bearing_x;
    std::int32_t bearing_y;
    std::int32_t advance;
    math::float2 uv1;
    math::float2 uv2;
};

class font
{
public:
    font(std::string_view font, std::size_t size);

    graphics::resource* texture() const noexcept { return m_texture.get(); }
    const glyph_data& glyph(char c) const noexcept { return m_glyph[c]; }

    std::uint32_t heigth() const noexcept { return m_heigth; }

private:
    std::unique_ptr<graphics::resource> m_texture;
    std::array<glyph_data, 128> m_glyph;

    std::uint32_t m_heigth;
};
} // namespace ash::ui