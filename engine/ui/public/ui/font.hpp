#pragma once

#include "graphics/render_device.hpp"
#include "math/math.hpp"
#include <memory>
#include <string_view>
#include <unordered_map>

namespace violet
{
struct glyph_data
{
    std::uint32_t width;
    std::uint32_t height;
    std::int32_t bearing_x;
    std::int32_t bearing_y;
    std::int32_t advance;
    float2 uv1;
    float2 uv2;
};

class font
{
public:
    font(std::string_view font, std::size_t size);

    rhi_texture* texture() const noexcept { return m_texture.get(); }
    const glyph_data& glyph(std::uint32_t character) const;

    std::uint32_t heigth() const noexcept { return m_heigth; }

private:
    rhi_ptr<rhi_texture> m_texture;
    std::unordered_map<std::uint32_t, glyph_data> m_glyph;

    std::uint32_t m_heigth;
};
} // namespace violet