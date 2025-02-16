#include "graphics/resources/texture.hpp"
#include "tools/texture_loader.hpp"

namespace violet
{
void raw_texture::set_texture(const rhi_texture_desc& desc)
{
    m_texture = render_device::instance().create_texture(desc);
}

void raw_texture::set_texture(rhi_ptr<rhi_texture>&& texture)
{
    m_texture = std::move(texture);
}

texture_2d::texture_2d(
    rhi_texture_extent extent,
    rhi_format format,
    rhi_texture_flags flags,
    std::uint32_t level_count,
    rhi_sample_count samples,
    rhi_texture_layout layout)
{
    set_texture({
        .extent = extent,
        .format = format,
        .flags = flags,
        .level_count = level_count,
        .layer_count = 1,
        .samples = samples,
        .layout = layout,
    });
}

texture_2d::texture_2d(std::string_view path, texture_options options)
{
    set_texture(texture_loader::load(path, options));
}

texture_2d::texture_2d(const texture_data& data, texture_options options)
{
    set_texture(texture_loader::load(data, options));
}

texture_cube::texture_cube(
    const rhi_texture_extent& extent,
    rhi_format format,
    rhi_texture_flags flags,
    std::uint32_t level_count,
    rhi_sample_count samples,
    rhi_texture_layout layout)
{
    set_texture({
        .extent = extent,
        .format = format,
        .flags = flags | RHI_TEXTURE_CUBE,
        .level_count = level_count,
        .layer_count = 6,
        .samples = samples,
        .layout = layout,
    });
}

texture_cube::texture_cube(
    std::string_view right,
    std::string_view left,
    std::string_view top,
    std::string_view bottom,
    std::string_view front,
    std::string_view back,
    texture_options options)
{
    set_texture(texture_loader::load(right, left, top, bottom, front, back, options));
}
} // namespace violet