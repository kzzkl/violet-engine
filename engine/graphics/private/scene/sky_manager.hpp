#pragma once

#include "graphics/atmosphere.hpp"
#include "graphics/render_interface.hpp"

namespace violet
{

class sky_manager
{
public:
    void set_skybox(
        rhi_texture* environment_map,
        rhi_buffer* irradiance_sh,
        rhi_texture* prefilter_map)
    {
        m_sky_type = SKY_TYPE_SKYBOX;
        m_environment_map = environment_map;
        // m_irradiance_map = irradiance_map;
        m_prefilter_map = prefilter_map;
    }

    void set_atmosphere(const atmosphere& atmosphere, rhi_texture* transmittance_lut)
    {
        m_sky_type = SKY_TYPE_ATMOSPHERE;
        m_atmosphere = atmosphere;
    }

private:
    sky_type m_sky_type{SKY_TYPE_NONE};

    rhi_texture* m_environment_map;
    rhi_buffer* m_irradiance_sh;
    rhi_texture* m_prefilter_map;

    atmosphere m_atmosphere;
};
} // namespace violet