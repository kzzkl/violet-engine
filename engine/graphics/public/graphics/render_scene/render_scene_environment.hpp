#pragma once

#include "graphics/atmosphere.hpp"
#include "graphics/render_device.hpp"
#include "graphics/render_scene/render_scene_module.hpp"

namespace violet
{
class render_scene_environment : public render_scene_module
{
public:
    void set_skybox(
        rhi_texture* environment_map,
        rhi_buffer* irradiance_sh,
        rhi_texture* prefilter_map);

    void set_atmosphere(
        const atmosphere& atmosphere,
        render_id sun_id,
        rhi_texture* transmittance_lut,
        rhi_texture* multi_scattering_lut);

    void update(render_scene_context& context, gpu_buffer_uploader& uploader) override;
    void reset() override
    {
        m_atmosphere_dirty = false;
    }

    rhi_texture* get_environment_map() const noexcept
    {
        return m_environment_map;
    }

    rhi_buffer* get_irradiance_sh() const noexcept
    {
        return m_irradiance_sh;
    }

    rhi_texture* get_prefilter_map() const noexcept
    {
        return m_prefilter_map;
    }

    const atmosphere& get_atmosphere() const noexcept
    {
        return m_atmosphere;
    }

    render_id get_sun_id() const noexcept
    {
        return m_sun_id;
    }

    const vec3f& get_sun_direction() const noexcept
    {
        return m_sun_direction;
    }

    const vec3f& get_sun_irradiance() const noexcept
    {
        return m_sun_irradiance;
    }

    rhi_texture* get_transmittance_lut() const noexcept
    {
        return m_transmittance_lut;
    }

    rhi_texture* get_multi_scattering_lut() const noexcept
    {
        return m_multi_scattering_lut;
    }

    bool is_atmosphere_dirty() const noexcept
    {
        return m_atmosphere_dirty;
    }

private:
    rhi_texture* m_environment_map;
    rhi_buffer* m_irradiance_sh;
    rhi_texture* m_prefilter_map;

    atmosphere m_atmosphere;
    render_id m_sun_id{INVALID_RENDER_ID};
    vec3f m_sun_direction;
    vec3f m_sun_irradiance;
    rhi_texture* m_transmittance_lut;
    rhi_texture* m_multi_scattering_lut;

    bool m_atmosphere_dirty{false};
};
} // namespace violet