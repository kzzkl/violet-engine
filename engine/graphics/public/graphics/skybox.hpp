#pragma once

#include "graphics/resources/texture.hpp"

namespace violet
{
class skybox
{
public:
    skybox(
        std::string_view path,
        const rhi_texture_extent& texture_extent = {512, 512},
        const rhi_texture_extent& irradiance_extent = {32, 32},
        const rhi_texture_extent& prefilter_extent = {256, 256});

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

private:
    std::unique_ptr<texture_cube> m_texture;
    std::unique_ptr<texture_cube> m_irradiance;
    std::unique_ptr<texture_cube> m_prefilter;
};
} // namespace violet
