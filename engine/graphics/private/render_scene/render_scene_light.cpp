#include "graphics/render_scene/render_scene_light.hpp"
#include "gpu_buffer_uploader.hpp"

namespace violet
{
render_id render_scene_light::add_light(std::uint32_t type)
{
    return m_lights.add();
}

void render_scene_light::remove_light(render_id light_id)
{
    m_lights.remove(light_id);
}

void render_scene_light::set_light_data(
    render_id light_id,
    const vec3f& color,
    const vec3f& position,
    const vec3f& direction)
{
    auto& light = m_lights[light_id];

    light.color = color;
    light.position = position;
    light.direction = direction;

    m_lights.mark_dirty(light_id);
}

void render_scene_light::set_light_shadow(render_id light_id, bool cast_shadow)
{
    auto& light = m_lights[light_id];

    light.cast_shadow = cast_shadow;

    m_lights.mark_dirty(light_id);
}

void render_scene_light::update(render_scene_context& context, gpu_buffer_uploader& uploader)
{
    m_lights.update(
        [&](const light_data& light) -> shader::light_data
        {
            return {
                .position = light.position,
                .type = light.type,
                .direction = light.direction,
                .cast_shadow = light.cast_shadow ? 1u : 0u,
                .color = light.color,
            };
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            uploader.upload(
                buffer,
                data,
                size,
                offset,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT |
                    RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });

    context.scene_data.light_buffer = m_lights.get_buffer()->get_srv()->get_bindless();
    context.scene_data.light_count = m_lights.get_size();
}
} // namespace violet