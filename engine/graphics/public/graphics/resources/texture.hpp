#pragma once

#include "graphics/render_device.hpp"

namespace violet
{
class raw_texture
{
public:
    raw_texture() = default;
    raw_texture(const raw_texture&) = delete;

    raw_texture& operator=(const raw_texture&) = delete;

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

    operator bool() const noexcept
    {
        return m_texture != nullptr;
    }

protected:
    void set_texture(const rhi_texture_desc& desc);
    void set_texture(rhi_ptr<rhi_texture>&& texture);

private:
    rhi_ptr<rhi_texture> m_texture;
};

struct texture_data
{
    rhi_format format;
    rhi_extent extent;

    std::uint32_t layer_count;
    std::uint32_t level_count;

    std::vector<char> pixels;
};

class texture_2d : public raw_texture
{
public:
    texture_2d() = default;
    texture_2d(
        rhi_extent extent,
        rhi_format format,
        rhi_texture_flags flags,
        std::uint32_t level_count = 1,
        std::uint32_t layer_count = 1,
        rhi_sample_count samples = RHI_SAMPLE_COUNT_1,
        rhi_texture_layout layout = RHI_TEXTURE_LAYOUT_UNDEFINED);

    texture_2d(const texture_data& data);
};

class texture_cube : public raw_texture
{
public:
    texture_cube(
        rhi_extent extent,
        rhi_format format,
        rhi_texture_flags flags,
        std::uint32_t level_count = 1,
        rhi_sample_count samples = RHI_SAMPLE_COUNT_1,
        rhi_texture_layout layout = RHI_TEXTURE_LAYOUT_UNDEFINED);

    texture_cube(const texture_data& data);
};
} // namespace violet