#pragma once

#include "graphics/resources/texture.hpp"

namespace violet
{
class skybox
{
public:
    struct atmosphere_data
    {
        vec3f rayleigh_scattering{5.802e-6f, 13.558e-6f, 33.1e-6f};
        float rayleigh_density_height{8000.0f};
        float mie_scattering{3.996e-6f};
        float mie_asymmetry{0.8f};
        float mie_absorption{4.4e-6f};
        float mie_density_height{1200.0f};
        vec3f ozone_absorption{0.65e-6f, 1.881e-6f, 0.085e-6f};
        float ozone_center_height{25000.0f};
        float ozone_width{30000.0f};
        float planet_radius{6360000.0f};
        float atmosphere_radius{6460000.0f};
        std::uint32_t padding0;
    };

    skybox(
        const rhi_extent& irradiance_extent = {.width = 32, .height = 32},
        const rhi_extent& prefilter_extent = {.width = 256, .height = 256},
        const rhi_extent& skybox_texture_extent = {.width = 512, .height = 512},
        const rhi_extent& transmittance_lut_extent = {.width = 256, .height = 64});

    void set_dynamic_sky(bool dynamic_sky);

    void set_skybox_texture(std::string_view path);

    texture_cube* get_skybox_texture() const noexcept
    {
        return m_skybox_texture.get();
    }

    std::uint32_t get_skybox_texture_bindless() const noexcept
    {
        return m_skybox_texture->get_srv(RHI_TEXTURE_DIMENSION_CUBE)->get_bindless();
    }

    void set_rayleigh_scattering(const vec3f& rayleigh_scattering)
    {
        m_atmosphere.rayleigh_scattering = rayleigh_scattering;
        m_dirty = true;
    }

    vec3f get_rayleigh_scattering() const noexcept
    {
        return m_atmosphere.rayleigh_scattering;
    }

    void set_rayleigh_density_height(float rayleigh_density_height)
    {
        m_atmosphere.rayleigh_density_height = rayleigh_density_height;
        m_dirty = true;
    }

    float get_rayleigh_density_height() const noexcept
    {
        return m_atmosphere.rayleigh_density_height;
    }

    void set_mie_scattering(float mie_scattering)
    {
        m_atmosphere.mie_scattering = mie_scattering;
        m_dirty = true;
    }

    float get_mie_scattering() const noexcept
    {
        return m_atmosphere.mie_scattering;
    }

    void set_mie_asymmetry(float mie_asymmetry)
    {
        m_atmosphere.mie_asymmetry = mie_asymmetry;
        m_dirty = true;
    }

    float get_mie_asymmetry() const noexcept
    {
        return m_atmosphere.mie_asymmetry;
    }

    void set_mie_absorption(float mie_absorption)
    {
        m_atmosphere.mie_absorption = mie_absorption;
        m_dirty = true;
    }

    float get_mie_absorption() const noexcept
    {
        return m_atmosphere.mie_absorption;
    }

    void set_mie_density_height(float mie_density_height)
    {
        m_atmosphere.mie_density_height = mie_density_height;
        m_dirty = true;
    }

    float get_mie_density_height() const noexcept
    {
        return m_atmosphere.mie_density_height;
    }

    void set_ozone_absorption(const vec3f& ozone_absorption)
    {
        m_atmosphere.ozone_absorption = ozone_absorption;
        m_dirty = true;
    }

    vec3f get_ozone_absorption() const noexcept
    {
        return m_atmosphere.ozone_absorption;
    }

    void set_ozone_center_height(float ozone_center_height)
    {
        m_atmosphere.ozone_center_height = ozone_center_height;
        m_dirty = true;
    }

    float get_ozone_center_height() const noexcept
    {
        return m_atmosphere.ozone_center_height;
    }

    void set_ozone_width(float ozone_width)
    {
        m_atmosphere.ozone_width = ozone_width;
        m_dirty = true;
    }

    float get_ozone_width() const noexcept
    {
        return m_atmosphere.ozone_width;
    }

    void set_planet_radius(float planet_radius)
    {
        m_atmosphere.planet_radius = planet_radius;
        m_dirty = true;
    }

    float get_planet_radius() const noexcept
    {
        return m_atmosphere.planet_radius;
    }

    void set_atmosphere_height(float atmosphere_height)
    {
        m_atmosphere.atmosphere_radius = m_atmosphere.planet_radius + atmosphere_height;
        m_dirty = true;
    }

    float get_atmosphere_height() const noexcept
    {
        return m_atmosphere.atmosphere_radius - m_atmosphere.planet_radius;
    }

    float get_atmosphere_radius() const noexcept
    {
        return m_atmosphere.atmosphere_radius;
    }

    const atmosphere_data& get_atmosphere() const noexcept
    {
        return m_atmosphere;
    }

    void set_sun_angular_radius(float sun_angular_radius)
    {
        m_sun_angular_radius = sun_angular_radius;
    }

    float get_sun_angular_radius() const noexcept
    {
        return m_sun_angular_radius;
    }

    texture_2d* get_transmittance_lut() const noexcept
    {
        return m_transmittance_lut.get();
    }

    std::uint32_t get_transmittance_lut_bindless() const noexcept
    {
        return m_transmittance_lut->get_srv()->get_bindless();
    }

    texture_cube* get_irradiance() const noexcept
    {
        return m_irradiance.get();
    }

    std::uint32_t get_irradiance_bindless() const noexcept
    {
        return m_irradiance->get_srv(RHI_TEXTURE_DIMENSION_CUBE)->get_bindless();
    }

    texture_cube* get_prefilter() const noexcept
    {
        return m_prefilter.get();
    }

    std::uint32_t get_prefilter_bindless() const noexcept
    {
        return m_prefilter->get_srv(RHI_TEXTURE_DIMENSION_CUBE)->get_bindless();
    }

    bool is_dynamic_sky() const noexcept
    {
        return m_dynamic_sky;
    }

    bool is_dirty() const noexcept
    {
        return m_dirty;
    }

    void update(rhi_command* command);

private:
    void update_texture(rhi_command* command);
    void update_atmosphere(rhi_command* command);

    std::unique_ptr<texture_cube> m_irradiance;
    rhi_extent m_irradiance_extent;

    std::unique_ptr<texture_cube> m_prefilter;
    rhi_extent m_prefilter_extent;

    bool m_dynamic_sky{true};

    std::string m_skybox_texture_path;

    std::unique_ptr<texture_cube> m_skybox_texture;
    rhi_extent m_skybox_texture_extent;

    atmosphere_data m_atmosphere;
    float m_sun_angular_radius{0.00465f};

    std::unique_ptr<texture_2d> m_transmittance_lut;
    rhi_extent m_transmittance_lut_extent;

    bool m_dirty;
};
} // namespace violet
