#pragma once

#include "graphics/render_device.hpp"

namespace violet
{
class raw_texture
{
public:
    virtual ~raw_texture() = default;

    rhi_texture_srv* get_srv(
        rhi_texture_dimension dimension = RHI_TEXTURE_DIMENSION_2D,
        std::uint32_t level = 0,
        std::uint32_t level_count = 0,
        std::uint32_t layer = 0,
        std::uint32_t layer_count = 0) const noexcept
    {
        return m_texture->get_srv(dimension, level, level_count, layer, layer_count);
    }

    rhi_texture_uav* get_uav(
        rhi_texture_dimension dimension = RHI_TEXTURE_DIMENSION_2D,
        std::uint32_t level = 0,
        std::uint32_t level_count = 0,
        std::uint32_t layer = 0,
        std::uint32_t layer_count = 0) const noexcept
    {
        return m_texture->get_uav(dimension, level, level_count, layer, layer_count);
    }

    rhi_texture* get_rhi() const noexcept
    {
        return m_texture.get();
    }

protected:
    void set_texture(const rhi_texture_desc& desc);
    void set_texture(rhi_ptr<rhi_texture>&& texture);

private:
    rhi_ptr<rhi_texture> m_texture;
};

enum texture_option
{
    TEXTURE_OPTION_NONE = 0,
    TEXTURE_OPTION_GENERATE_MIPMAPS = 1 << 0,
    TEXTURE_OPTION_SRGB = 1 << 1,
};
using texture_options = std::uint32_t;

struct texture_data
{
    struct mipmap
    {
        rhi_texture_extent extent;
        std::vector<char> pixels;
    };

    std::vector<mipmap> mipmaps;
    rhi_format format;
};

class texture_2d : public raw_texture
{
public:
    texture_2d(
        rhi_texture_extent extent,
        rhi_format format,
        rhi_texture_flags flags,
        std::uint32_t level_count = 1,
        rhi_sample_count samples = RHI_SAMPLE_COUNT_1,
        rhi_texture_layout layout = RHI_TEXTURE_LAYOUT_UNDEFINED);

    texture_2d(std::string_view path, texture_options options = TEXTURE_OPTION_NONE);
    texture_2d(const texture_data& data, texture_options options = TEXTURE_OPTION_NONE);
};

class texture_cube : public raw_texture
{
public:
    texture_cube(
        const rhi_texture_extent& extent,
        rhi_format format,
        rhi_texture_flags flags,
        std::uint32_t level_count = 1,
        rhi_sample_count samples = RHI_SAMPLE_COUNT_1,
        rhi_texture_layout layout = RHI_TEXTURE_LAYOUT_UNDEFINED);

    texture_cube(
        std::string_view right,
        std::string_view left,
        std::string_view top,
        std::string_view bottom,
        std::string_view front,
        std::string_view back,
        texture_options options = TEXTURE_OPTION_NONE);
};
} // namespace violet