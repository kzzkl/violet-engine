#include "graphics/resources/texture.hpp"
#include "graphics/texture_loader.hpp"

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
    rhi_extent extent,
    rhi_format format,
    rhi_texture_flags flags,
    std::uint32_t level_count,
    std::uint32_t layer_count,
    rhi_sample_count samples,
    rhi_texture_layout layout)
{
    set_texture({
        .extent = extent,
        .format = format,
        .flags = flags,
        .level_count = level_count,
        .layer_count = layer_count,
        .samples = samples,
        .layout = layout,
    });
}

texture_2d::texture_2d(const texture_data& data)
{
    set_texture(texture_loader::load(data));
}

texture_cube::texture_cube(
    rhi_extent extent,
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

texture_cube::texture_cube(const texture_data& data)
{
    set_texture(texture_loader::load(data, texture_loader::LOAD_OPTION_CUBE_MAP));
}
} // namespace violet