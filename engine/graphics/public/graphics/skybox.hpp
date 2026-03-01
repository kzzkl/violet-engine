#pragma once

#include "graphics/atmosphere.hpp"
#include "graphics/resources/texture.hpp"

namespace violet
{
class skybox
{
public:
    skybox(
        std::string_view path,
        const rhi_texture_extent& texture_extent = {.width = 512, .height = 512},
        const rhi_texture_extent& irradiance_extent = {.width = 32, .height = 32},
        const rhi_texture_extent& prefilter_extent = {.width = 256, .height = 256});

    texture_cube* get_texture() const noexcept
    {
        return m_texture.get();
    }

    texture_cube* get_irradiance() const noexcept
    {
        return m_irradiance.get();
    }

    texture_cube* get_prefilter() const noexcept
    {
        return m_prefilter.get();
    }

    void set_atmosphere(const atmosphere& atmosphere)
    {
        m_atmosphere = atmosphere;
    }

    const atmosphere& get_atmosphere() const noexcept
    {
        return m_atmosphere;
    }

private:
    std::unique_ptr<texture_cube> m_texture;
    std::unique_ptr<texture_cube> m_irradiance;
    std::unique_ptr<texture_cube> m_prefilter;

    atmosphere m_atmosphere;
};
} // namespace violet
