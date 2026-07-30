#include "light_system.hpp"
#include "components/light_component.hpp"
#include "components/light_component_meta.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/render_scene/render_scene_light.hpp"
#include "graphics/render_scene/render_scene_shadow.hpp"

namespace violet
{
light_system::light_system()
    : system("light")
{
}

bool light_system::initialize(const dictionary& config)
{
    auto& world = get_world();
    world.register_component<light_component>();
    world.register_component<light_component_meta>();

    return true;
}

void light_system::update(render_scene_manager& scene_manager)
{
    auto& world = get_world();

    world.get_view()
        .read<scene_component>()
        .read<light_component>()
        .read<transform_world_component>()
        .write<light_component_meta>()
        .each(
            [&](const scene_component& scene,
                const light_component& light,
                const transform_world_component& transform,
                light_component_meta& light_meta)
            {
                render_scene* render_scene = scene_manager.get_scene(scene.layer);

                auto& light_module = render_scene->get_module<render_scene_light>();
                auto& shadow_module = render_scene->get_module<render_scene_shadow>();

                if (light_meta.scene != render_scene)
                {
                    if (light_meta.scene != nullptr)
                    {
                        if (light_meta.cast_shadow)
                        {
                            light_meta.scene->get_module<render_scene_shadow>().remove_shadow(
                                light_meta.id);
                        }
                        light_meta.scene->get_module<render_scene_light>().remove_light(
                            light_meta.id);
                    }

                    light_meta.id = light_module.add_light(light.type);
                    light_meta.scene = render_scene;

                    if (light.cast_shadow)
                    {
                        shadow_module.add_shadow(light_meta.id);
                    }
                    light_module.set_light_shadow(light_meta.id, light.cast_shadow);
                    light_meta.cast_shadow = light.cast_shadow;
                }

                light_module.set_light_data(
                    light_meta.id,
                    light.color,
                    transform.get_position(),
                    transform.get_forward());

                if (light.cast_shadow != light_meta.cast_shadow)
                {
                    if (light.cast_shadow)
                    {
                        shadow_module.add_shadow(light_meta.id);
                    }
                    else
                    {
                        shadow_module.remove_shadow(light_meta.id);
                    }

                    light_module.set_light_shadow(light_meta.id, light.cast_shadow);

                    light_meta.cast_shadow = light.cast_shadow;
                }
            },
            [this](auto& view)
            {
                return view.template is_updated<light_component>(m_system_version) ||
                       view.template is_updated<scene_component>(m_system_version) ||
                       view.template is_updated<transform_world_component>(m_system_version);
            });

    m_system_version = world.get_version();
}
} // namespace violet