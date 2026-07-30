#pragma once

#include "components/light_component.hpp"
#include "graphics/gpu_array.hpp"
#include "graphics/render_scene/render_scene_module.hpp"

namespace violet
{
class gpu_buffer_uploader;

class render_scene_light : public render_scene_module
{
public:
    struct light_data
    {
        using gpu_type = shader::light_data;

        light_type type;
        vec3f position;
        vec3f direction;
        vec3f color;
        bool cast_shadow;
    };

    render_id add_light(std::uint32_t type);
    void remove_light(render_id light_id);
    void set_light_data(
        render_id light_id,
        const vec3f& color,
        const vec3f& position,
        const vec3f& direction);
    void set_light_shadow(render_id light_id, bool cast_shadow);

    void update(render_scene_context& context, gpu_buffer_uploader& uploader) override;

    const light_data& get_light(render_id light_id) const
    {
        return m_lights[light_id];
    }

    std::uint32_t get_light_count() const noexcept
    {
        return m_lights.get_size();
    }

private:
    void update_light(gpu_buffer_uploader* uploader);

    gpu_dense_array<light_data> m_lights;
};
} // namespace violet