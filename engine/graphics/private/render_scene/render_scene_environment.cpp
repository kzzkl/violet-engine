#include "graphics/render_scene/render_scene_environment.hpp"
#include "graphics/render_scene/render_scene_light.hpp"

namespace violet
{
void render_scene_environment::set_skybox(
    rhi_texture* environment_map,
    rhi_buffer* irradiance_sh,
    rhi_texture* prefilter_map)
{
    m_environment_map = environment_map;
    m_irradiance_sh = irradiance_sh;
    m_prefilter_map = prefilter_map;
}

void render_scene_environment::set_atmosphere(
    const atmosphere& atmosphere,
    render_id sun_id,
    rhi_texture* transmittance_lut,
    rhi_texture* multi_scattering_lut)
{
    m_atmosphere = atmosphere;
    m_sun_id = sun_id;
    m_transmittance_lut = transmittance_lut;
    m_multi_scattering_lut = multi_scattering_lut;

    m_atmosphere_dirty = true;
}

void render_scene_environment::update(render_scene_context& context, gpu_buffer_uploader& uploader)
{
    if (m_sun_id == INVALID_RENDER_ID)
    {
        return;
    }

    const auto& light = context.get_module<render_scene_light>().get_light(m_sun_id);

    if (m_sun_direction != light.direction || m_sun_irradiance != light.color)
    {
        m_atmosphere_dirty = true;
    }

    m_sun_direction = light.direction;
    m_sun_irradiance = light.color;
}
} // namespace violet