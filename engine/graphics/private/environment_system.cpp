#include "environment_system.hpp"
#include "components/light_component.hpp"
#include "components/light_component_meta.hpp"
#include "components/scene_component.hpp"
#include "components/skybox_component.hpp"

namespace violet
{
environment_system::environment_system()
    : system("environment")
{
}

bool environment_system::initialize(const dictionary& config)
{
    auto& world = get_world();
    world.register_component<skybox_component>();

    return true;
}

void environment_system::update(render_scene_manager& scene_manager)
{
    auto& world = get_world();

    world.get_view()
        .read<skybox_component>()
        .read<light_component_meta>()
        .read<scene_component>()
        .each(
            [&](const skybox_component& skybox,
                const light_component_meta& light_meta,
                const scene_component& scene)
            {
                if (skybox.skybox->is_dirty())
                {
                    m_update_queue.push_back(skybox.skybox);
                }

                render_scene* render_scene = scene_manager.get_scene(scene.layer);
                render_scene->set_skybox(skybox.skybox);

                if (skybox.skybox->is_dynamic_sky())
                {
                    render_scene->set_sun(light_meta.id);
                }
            },
            [this](auto& view)
            {
                return view.template is_updated<skybox_component>(m_system_version) ||
                       view.template is_updated<light_component>(m_system_version);
            });

    m_system_version = world.get_version();
}

void environment_system::record(rhi_command* command)
{
    for (auto* skybox : m_update_queue)
    {
        if (skybox->is_dirty())
        {
            skybox->update(command);
        }
    }
}
} // namespace violet