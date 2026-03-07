#include "graphics/skybox.hpp"
#include "tools/atmosphere_tool.hpp"
#include "tools/ibl_tool.hpp"

namespace violet
{
skybox::skybox(
    const rhi_extent& irradiance_extent,
    const rhi_extent& prefilter_extent,
    const rhi_extent& skybox_texture_extent,
    const rhi_extent& transmittance_lut_extent)
    : m_irradiance_extent(irradiance_extent),
      m_prefilter_extent(prefilter_extent),
      m_skybox_texture_extent(skybox_texture_extent),
      m_transmittance_lut_extent(transmittance_lut_extent)
{
}

void skybox::set_dynamic_sky(bool dynamic_sky)
{
    m_dynamic_sky = dynamic_sky;
    m_dirty = true;

    if (m_dynamic_sky)
    {
        if (m_transmittance_lut == nullptr)
        {
            m_transmittance_lut = std::make_unique<texture_2d>(
                m_transmittance_lut_extent,
                RHI_FORMAT_R11G11B10_FLOAT,
                RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE);
            m_transmittance_lut->get_rhi()->set_name("Transmittance LUT");
        }
    }
    else
    {
    }
}

void skybox::set_skybox_texture(std::string_view path)
{
    if (m_skybox_texture_path != path)
    {
        m_skybox_texture_path = path;
        m_dirty = true;
    }

    if (m_skybox_texture == nullptr)
    {
        auto get_level_count = [](rhi_extent extent) -> std::uint32_t
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

        m_skybox_texture = std::make_unique<texture_cube>(
            m_skybox_texture_extent,
            RHI_FORMAT_R11G11B10_FLOAT,
            RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_SRC |
                RHI_TEXTURE_TRANSFER_DST | RHI_TEXTURE_CUBE,
            get_level_count(m_skybox_texture_extent));
        m_skybox_texture->get_rhi()->set_name("Skybox");
    }
}

void skybox::update(rhi_command* command)
{
    if (!m_dirty)
    {
        return;
    }

    m_dirty = false;

    if (m_irradiance == nullptr || m_irradiance->get_rhi()->get_extent() != m_irradiance_extent)
    {
        m_irradiance = std::make_unique<texture_cube>(
            m_irradiance_extent,
            RHI_FORMAT_R11G11B10_FLOAT,
            RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE);
        m_irradiance->get_rhi()->set_name("Skybox Irradiance");
    }

    if (m_prefilter == nullptr || m_prefilter->get_rhi()->get_extent() != m_prefilter_extent)
    {
        m_prefilter = std::make_unique<texture_cube>(
            m_prefilter_extent,
            RHI_FORMAT_R11G11B10_FLOAT,
            RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE);
        m_prefilter->get_rhi()->set_name("Skybox Prefilter");
    }

    if (m_dynamic_sky)
    {
        update_atmosphere(command);
    }
    else
    {
        update_texture(command);
    }
}

void skybox::update_texture(rhi_command* command)
{
    texture_2d env_map(m_skybox_texture_path);
    ibl_tool::generate_cube_map(env_map.get_rhi(), m_skybox_texture->get_rhi(), command);
    ibl_tool::generate_ibl(
        m_skybox_texture->get_rhi(),
        m_irradiance->get_rhi(),
        m_prefilter->get_rhi(),
        command);
}

void skybox::update_atmosphere(rhi_command* command)
{
    atmosphere_tool::generate_transmittance_lut(
        m_rayleigh_scattering,
        m_rayleigh_density_height,
        m_mie_scattering,
        m_mie_absorption,
        m_mie_density_height,
        m_ozone_center_height,
        m_ozone_absorption,
        m_ozone_width,
        m_planet_radius,
        m_atmosphere_height,
        1024,
        m_transmittance_lut->get_rhi(),
        command);
}
} // namespace violet