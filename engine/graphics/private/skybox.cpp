#include "graphics/skybox.hpp"
#include "tools/ibl_tool.hpp"

namespace violet
{
skybox::skybox(
    std::string_view path,
    const rhi_texture_extent& texture_extent,
    const rhi_texture_extent& irradiance_extent,
    const rhi_texture_extent& prefilter_extent)
{
    auto get_level_count = [](rhi_texture_extent extent) -> std::uint32_t
    {
        std::uint32_t level_count = 0;

        while (extent.height > 0 && extent.width > 0)
        {
            extent.height >>= 1;
            extent.width >>= 1;

            ++level_count;
        }

        return level_count;
    };

    m_texture = std::make_unique<texture_cube>(
        texture_extent,
        RHI_FORMAT_R11G11B10_FLOAT,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_SRC |
            RHI_TEXTURE_TRANSFER_DST | RHI_TEXTURE_CUBE,
        get_level_count(texture_extent));

    m_irradiance = std::make_unique<texture_cube>(
        irradiance_extent,
        RHI_FORMAT_R11G11B10_FLOAT,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE);

    m_prefilter = std::make_unique<texture_cube>(
        prefilter_extent,
        RHI_FORMAT_R11G11B10_FLOAT,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE,
        get_level_count(prefilter_extent));

    texture_2d env_map(path);
    ibl_tool::generate_cube_map(env_map.get_rhi(), m_texture->get_rhi());
    ibl_tool::generate_ibl(m_texture->get_rhi(), m_irradiance->get_rhi(), m_prefilter->get_rhi());
}
} // namespace violet